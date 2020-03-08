#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>

#include "storage.h"

using namespace std;

#define IP_LEN 16
#define TRUE 0
#define BUFLEN 1600
#define MAX_CLIENTS 33

int main(int argc, char *argv[])
{
	int socket_tcp, socket_udp, portno, client_len, newsockfd;
	char buf[BUFLEN];
	char ip_buf[IP_LEN];
	bool halt = false;
	int i, n, fd_max;
	fd_set read_fds, tmp_fds;
	message msg;
	string msg_topic;
	Storage storage;
	struct sockaddr_in server_addr, client_addr;
	socklen_t len = sizeof(struct sockaddr_in);

	if (argc < 2) {
		fprintf(stderr, "Usage: %s <server_port>\n", argv[0]);
		exit(-1);
	}

	portno = atoi(argv[1]);
	if (portno == 0) {
		fprintf(stderr, "Invalid port: %s\n", argv[1]);
		exit(-1);
	}

	// initializare adresa server
	memset((char *) &server_addr, 0, sizeof(server_addr));
	server_addr.sin_family      = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port        = htons(portno);

	// initializare socket TCP
	socket_tcp = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_tcp < 0) {
		perror("Error opening socket TCP");
		exit(-1);
	}

	int one = 1;
	if (setsockopt(socket_tcp, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one)) ==
	    -1) {
		perror("Error in setsockopt");
		exit(-1);
	};

	if (bind(socket_tcp, (struct sockaddr *) &server_addr,
	         sizeof(struct sockaddr)) < 0) {
		perror("Error on binding TCP");
		exit(-1);
	}

	if (listen(socket_tcp, MAX_CLIENTS) < 0) {
		perror("Error on listening");
		exit(-1);
	}

	// initializare socket UDP
	socket_udp = socket(AF_INET, SOCK_DGRAM, 0);
	if (socket_udp < 0) {
		perror("Error opening socket UDP");
		exit(-1);
	}

	if (bind(socket_udp, (struct sockaddr *) &server_addr,
	         sizeof(server_addr)) < 0) {
		perror("Error on binding UDP");
		exit(-1);
	}
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	FD_SET(0, &read_fds);
	FD_SET(socket_tcp, &read_fds);
	FD_SET(socket_udp, &read_fds);

	if (socket_tcp > socket_udp)
		fd_max = socket_tcp;
	else
		fd_max = socket_udp;

	while (!halt) {
		tmp_fds = read_fds;
		if (select(fd_max + 1, &tmp_fds, NULL, NULL, NULL) == -1)
			perror("Error in select");

		for (i = 0; i <= fd_max; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == 0) { // citire de la tastatura
					memset(buf, 0, BUFLEN);
					fgets(buf, BUFLEN - 1, stdin);
					// verificare mesaj
					if (!strcmp(buf, "exit\n")) { // inchidere server
						// initializare inchidere clienti
						for (i = 0; i <= fd_max; i++) {
							if (FD_ISSET(i, &tmp_fds)) {
								close(i);
							}
						}
						halt = true;
						break;
					}

				} else if (i == socket_udp) { // citire socket udp
					memset(buf, 0, BUFLEN);
					if (recvfrom(socket_udp, buf, BUFLEN, 0,
					             (struct sockaddr *) &server_addr, &len) < 0) {
						perror("Error in recvfrom");
						exit(-1);
					}
					inet_ntop(AF_INET, &(server_addr.sin_addr), ip_buf, IP_LEN);
					msg = storage.parse_msg_udp(buf, ip_buf,
					                            server_addr.sin_port);
					if (msg.valid)
						msg_topic = storage.create_message(msg);
					else
						continue;

					// verific daca este cineva conectat la topic
					auto it = storage.topics.find(msg.topic);
					if (it != storage.topics.end()) { // topicul exista
						// pentru fiecare client connectat la topicul curent
						for (sub_client sub : it->second) {
							// caut clientul in clients pentru a vedea daca e
							// conectat
							auto cl = storage.clients.find(sub.client_ID);
							if (cl != storage.clients.end()) {
								if (cl->second.connected) {
									// trimitere mesaj
									if (send(cl->second.fd_client,
									         msg_topic.c_str(),
									         msg_topic.size(), 0) < 0) {
										perror("Error writing to socket");
									}
								} else { // client deconectat
									// daca are activa setarea de store and
									// forward memorez mesajul
									if (sub.SF) {
										cl->second.messages.push_back(
										    msg_topic);
									}
								}
							}
						}
					}
					// daca primesc mesaj pe socketul de TCP
				} else if (i == socket_tcp) { // primire clienti noi
					client_len = sizeof(client_addr);
					newsockfd =
					    accept(socket_tcp, (struct sockaddr *) &client_addr,
					           (socklen_t *) &client_len);
					if (newsockfd == -1) {
						perror("Error in accept");
						exit(-1);
					} else {
						// adaugare socket nou (intors de accept) la
						// multimea descriptorilor de citire
						FD_SET(newsockfd, &read_fds);
						if (newsockfd > fd_max)
							fd_max = newsockfd;
					}
					inet_ntop(AF_INET, &(client_addr.sin_addr), ip_buf, IP_LEN);
					date client_date;
					client_date.ip_client_tcp   = ip_buf;
					client_date.port_client_tcp = client_addr.sin_port;

					// memorez datele noului client
					storage.new_client(newsockfd, client_date);

				} else { // citire socket tcp
					memset(buf, 0, BUFLEN);
					if ((n = recv(i, buf, sizeof(buf), 0)) <= 0) {
						if (n == 0) { // deconectare client
							storage.disconnect_client(i);
						} else {
							perror("Error in recv");
							exit(-1);
						}
						close(i);
						FD_CLR(i, &read_fds);
					} else { // primire mesaj client tcp
						string result = storage.parse_msg_tcp(i, buf);
						if (result == "connect") {
							// trimit masajele memorate cat timp a fost
							// deconectat
							date conn_client = storage.client_connected[i];
							string client_ID = conn_client.client_ID;
							auto cl          = storage.clients.find(client_ID);
							if (cl != storage.clients.end()) {
								int size = cl->second.messages.size();
								for (int k = 0; k < size; k++) {
									if (send(i, cl->second.messages[k].c_str(),
									         cl->second.messages[k].size(),
									         0) < 0) {
										perror("Error writing to socket.");
									}
								}
								cl->second.messages.clear();
							}
						}
					}
				}
			}
		}
	}
	// close(socket_udp);
	// close(socket_tcp);

	return 0;
}
