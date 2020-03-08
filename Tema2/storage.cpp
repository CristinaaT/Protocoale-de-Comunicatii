#include "storage.h"

#include <math.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

using namespace std;

Storage::Storage() {}

message Storage::parse_msg_udp(char *package, string ip_client, int port_client)
{
	string topic;
	message msg;

	msg.ip_client_udp   = ip_client;
	msg.port_client_udp = port_client;

	// aflare tip mesaj si payload
	if (package[TOPIC_SIZE] == 0) {
		msg.type = "INT";

		uint32_t number;
		// aflare numar si tranformare in format pentru host
		memcpy(&number, package + TOPIC_SIZE + 2, sizeof(uint32_t));
		number = ntohl(number);

		msg.payload = to_string(number);
		// determinare semn
		if (package[TOPIC_SIZE + 1] == 1) {
			msg.payload.insert(0, "-");
		}

	} else if (package[TOPIC_SIZE] == 1) {
		msg.type = "SHART_REAL";
		uint16_t number;
		// aflare numar si tranformare in format pentru host
		memcpy(&number, package + TOPIC_SIZE + 1, sizeof(uint16_t));
		number = ntohs(number);

		stringstream stream;
		stream << fixed << setprecision(2) << 1.0 * number / 100;
		msg.payload = stream.str();

	} else if (package[TOPIC_SIZE] == 2) {
		msg.type = "FLAOT";
		uint32_t number;
		uint8_t power_t;
		int power;

		// aflare numar si tranformare in format pentru host
		memcpy(&number, package + TOPIC_SIZE + 2, sizeof(uint32_t));
		number = ntohl(number);

		// aflare putere
		memcpy(&power_t, package + TOPIC_SIZE + 2 + sizeof(uint32_t),
		       sizeof(uint8_t));
		power = (int) power_t;

		stringstream stream;
		// determinare semn
		if (package[TOPIC_SIZE + 1] == 1) {
			stream << "-";
		}
		stream << fixed << setprecision(power) << 1.0 * number / pow(10, power);

		msg.payload = stream.str();

	} else if (package[TOPIC_SIZE] == 3) {
		msg.type    = "STRING";
		msg.payload = package + TOPIC_SIZE + 1;

	} else {
		// tip payload invalid
		msg.valid = false;
		return msg;
	}

	// pentru determinarea topicu-ului se inlocuieste caracterul 50 cu \0 pentru
	// cazul in care topicul are exact 50 de caractere
	package[50] = '\0';

	char *token;
	token     = strtok(package, "\0");
	topic     = token;
	msg.topic = topic;
	msg.valid = true;

	return msg;
}

string Storage::create_message(message msg)
{
	stringstream stream;
	stream << msg.ip_client_udp << ":" << msg.port_client_udp << " - "
	       << msg.topic << " - " << msg.type << " - " << msg.payload << "\n";
	return stream.str();
}

string Storage::parse_msg_tcp(int client_fd, char *package)
{
	char *token;
	string client_ID, action, topic;
	date client_date;

	token = strtok(package, " ");
	if (token == NULL)
		return "error";

	client_ID = token;

	token = strtok(NULL, " ");
	if (token == NULL)
		return "error";

	action = token;
	if (action == "connect") {
		// afisare conectare client
		client_date                 = client_connected[client_fd];
		client_date.client_ID       = client_ID;
		client_connected[client_fd] = client_date;
		cout << "New client " << client_ID << " connected from "
		     << client_date.ip_client_tcp << ":" << client_date.port_client_tcp
		     << endl;

		// verific daca este un client noul
		auto it = clients.find(client_ID);
		if (it == clients.end()) {
			client new_client;
			new_client.connected = true;
			new_client.fd_client = client_fd;
			clients[client_ID]   = new_client;
		} else {
			// daca se conecteaza 2 clienti cu acelasi id, nu il actualizez
			if (!it->second.connected) {
				it->second.connected = true;
				it->second.fd_client = client_fd;
			}
		}
		return "connect";
	} else if (action == "subscribe") {
		token = strtok(NULL, " ");
		if (token == NULL)
			return "error";

		topic = token;

		token = strtok(NULL, " ");
		if (token == NULL)
			return "error";
		bool sf;
		if (!strcmp(token, __SF)) {
			sf = true;
		} else {
			sf = false;
		}

		// verific daca exista topicul respectiv
		auto it = topics.find(topic);
		if (it == topics.end()) { // topicul nu exista
			// adaug noul topic cu noul client
			vector<sub_client> sub_clients;
			sub_client subs;
			subs.client_ID = client_ID;
			subs.SF        = sf;
			sub_clients.push_back(subs);
			topics[topic] = sub_clients;
		} else {
			// verific daca e deja abonat la topicul respectiv
			for (sub_client cl : it->second) {
				// daca clientul deja este abonat la topicul respectiv
				// doar voi actualiza setarea de store and forward
				if (cl.client_ID == client_ID) {
					cl.SF = sf;
					return "subscribe";
				}
			}

			// clientul nu este deja abonat, asa ca il voi adauga in lista de
			// clienti abonati
			sub_client subs;
			subs.client_ID = client_ID;
			subs.SF        = sf;
			it->second.push_back(subs);
		}

		return "subscribe";
	} else if (action == "unsubscribe") {
		token = strtok(NULL, " ");
		if (token == NULL)
			return "error";

		topic   = token;
		auto it = topics.find(topic);
		if (it != topics.end()) { // topicul exista
			// adaug noul topic cu noul client
			int n = it->second.size();
			// verific ca clientul sa fie abonat la topicul respectiv
			for (int i = 0; i < n; i++) {
				// daca clientul este abonat
				if (it->second[i].client_ID == client_ID) {
					it->second.erase(it->second.begin() + i);
					return "unsubsribe";
				}
			}
		}
		// verific daca e deja abonat la topicul respectiv

		return "unsubscribe";
	}
	return "invalud";
}

void Storage::new_client(int fd_client, date client_date)
{
	client_connected[fd_client] = client_date;
}

void Storage::disconnect_client(int fd_client)
{
	date client_date = client_connected[fd_client];
	cout << "Client " << client_date.client_ID << " disconnected\n";
	client_connected.erase(fd_client);

	// pun clientul ca fiind inactiv
	auto it = clients.find(client_date.client_ID);
	if (it != clients.end()) {
		it->second.connected = false;
	}
}
