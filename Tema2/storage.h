#include <stdio.h>
#include <string.h>

#include <iostream>
#include <unordered_map>
#include <vector>

#define TOPIC_SIZE 50
#define __SF "1"
using namespace std;
typedef struct message {
	string ip_client_udp;
	int port_client_udp;
	string type;
	string payload;
	string topic;
	bool valid;
} message;

typedef struct sub_client {
	string client_ID;
	bool SF;
} sub_client;

typedef struct client {
	bool connected;
	int fd_client;
	vector<string> messages;
} client;

typedef struct date {
	string ip_client_tcp;
	int port_client_tcp;
	string client_ID;
} date;

class Storage
{
 public:
	// file descriptor client -> client_ID
	unordered_map<int, date> client_connected;
	// topic -> client_ID, SF
	unordered_map<string, vector<sub_client>> topics;
	// client_ID -> connected, fd, messages
	unordered_map<string, client> clients;

	Storage();

	message parse_msg_udp(char *package, string ip_client, int port_client);
	string parse_msg_tcp(int client_fd, char *package);
	void new_client(int fd_client, date client_date);
	void disconnect_client(int fd_client);
	string create_message(message msg);
};
