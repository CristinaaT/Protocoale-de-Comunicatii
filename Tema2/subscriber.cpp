#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>

#define BUFLEN 1600
#define ID_LEN 10

using namespace std;

int main(int argc, char *argv[])
{
	int socket_tcp, portno, fd_max;
	struct sockaddr_in server_addr;
	char buf[BUFLEN];
	char send_buf[BUFLEN];
	char connect_v[] = "connect";
	bool halt        = false;
	char *client_ID;
	int length_ID, length, n;

	fd_set client_fds;
	fd_set tmp_fds;

	FD_ZERO(&client_fds);
	FD_ZERO(&tmp_fds);

	if (argc < 4) {
		fprintf(stderr, "Usage %s <ID_Client> <IP_Server>  <Port_Server>\n",
		        argv[0]);
		exit(-1);
	}

	client_ID = strndup(argv[1], ID_LEN);
	length_ID = strlen(client_ID);
	portno    = atoi(argv[3]);

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

	server_addr.sin_family = AF_INET;
	server_addr.sin_port   = htons(portno);
	if (inet_aton(argv[2], &server_addr.sin_addr) < 0) {
		perror("Error in inet_aton");
		close(socket_tcp);
		exit(-1);
	}

	if (connect(socket_tcp, (struct sockaddr *) &server_addr,
	            sizeof(server_addr)) < 0) {
		perror("Error connecting");
		close(socket_tcp);
		exit(-1);
	}

	memset(send_buf, 0, BUFLEN);
	strcpy(send_buf, client_ID);
	length             = length_ID;
	send_buf[length++] = ' ';
	strcpy(send_buf + length, connect_v);
	length += strlen(connect_v);
	send_buf[length++] = ' ';

	if (send(socket_tcp, send_buf, BUFLEN, 0) < 0) {
		perror("Error writing to socket.");
		exit(-1);
	}

	FD_SET(0, &client_fds);
	FD_SET(socket_tcp, &client_fds);
	fd_max = socket_tcp;

	while (!halt) {
		tmp_fds = client_fds;
		if (select(fd_max + 1, &tmp_fds, NULL, NULL, NULL) == -1) {
			perror("Error in select");
			break;
		}
		for (int i = 0; i <= fd_max; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == 0) {
					/* Citesc de la tastatura */
					memset(buf, 0, BUFLEN);
					fgets(buf, BUFLEN - 1, stdin);
					if (!strcmp(buf, "exit\n")) { // inchidere client
						halt = true;
						break;
					}

					memset(send_buf, 0, BUFLEN);
					strcpy(send_buf, client_ID);
					length             = length_ID;
					send_buf[length++] = ' ';

					char *token;
					string topic;
					token = strtok(buf, " \n");
					if (token == NULL)
						continue;

					if (!strcmp(token, "subscribe")) {
						// adaugare subsribe in mesaj
						strcpy(send_buf + length, token);
						length += strlen(token);
						send_buf[length++] = ' ';

						// adaugare topic in mesaj
						token = strtok(NULL, " \n");
						if (token == NULL)
							continue;

						topic = token;
						// nu exista topic mai mare de 50 de caractere
						if (topic.size() > 50)
							continue;

						strcpy(send_buf + length, token);
						length += strlen(token);
						send_buf[length++] = ' ';

						// adaugare SF in mesaj
						token = strtok(NULL, " \n");
						if (token == NULL)
							continue;

						if (!strcmp(token, "0")) {
							// pentru a nu fi confundat cu \0
							send_buf[length++] = '2';
						} else if (!strcmp(token, "1")) {
							send_buf[length++] = '1';
						} else
							continue;

						send_buf[length++] = ' ';

						// trimitere mesaj catre server
						if (send(socket_tcp, send_buf, BUFLEN, 0) < 0) {
							perror("Error writing to socket.");
							close(socket_tcp);
							exit(-1);
						}
						cout << "subscribe " << topic << endl;
					} else if (!strcmp(token, "unsubscribe")) {
						// adaugare unsubsribe in mesaj
						strcpy(send_buf + length, token);
						length += strlen(token);
						send_buf[length++] = ' ';

						// adaugare topic in mesaj
						token = strtok(NULL, " \n");
						if (token == NULL)
							continue;

						topic = token;
						// nu exista topic mai mare de 50 de caractere
						if (topic.size() > 50)
							continue;

						strcpy(send_buf + length, token);
						length += strlen(token);
						send_buf[length++] = ' ';

						// trimitere mesaj catre server
						if (send(socket_tcp, send_buf, BUFLEN, 0) < 0) {
							perror("Error writing to socket.");
							close(socket_tcp);
							exit(-1);
						}
						cout << "unsubscribe " << topic << endl;
					}
				} else if (i == socket_tcp) {
					memset(buf, 0, BUFLEN);
					n = recv(socket_tcp, buf, BUFLEN, 0);
					if (n == 0) { // serverul a inchis conexiunea
						halt = true;
						break;
					}
					if (n < 0) {
						perror("Error receving from socket");
						close(socket_tcp);
						exit(-1);
					}
					cout << buf;
				}
			}
		}
	}
	free(client_ID);
	close(socket_tcp);
	return 0;
}
