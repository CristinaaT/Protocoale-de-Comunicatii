#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "link_emulator/lib.h"
#include "window.h"

#define HOST "127.0.0.1"
#define PORT 10000

int main(int argc, char **argv)
{
	int file_descriptor = -1;
	int timeout         = TIMEOUT;
	int file_end_found  = 0;

	msg msg_tmp;
	window_packet win_packet;

	unsigned long win_index;
	unsigned long window_size;
	unsigned long nack_found = 0;

	unsigned long last_sent      = 0;
	unsigned long last_confirmed = 0;

	/* Check the number of arguments and their type. */
	if (argc != 4 || atoi(argv[2]) == 0 || atoi(argv[3]) == 0)
		return EXIT_FAILURE;

	/* Calculate optimal window size: speed * RTT / msg_size / 8 */
	window_size = atoi(argv[2]) * 1000 * 2 / 8 * atoi(argv[3]) / sizeof(msg);

	/* Open the file. */
	file_descriptor = open(argv[1], O_RDONLY);
	if (file_descriptor == -1)
		return EXIT_FAILURE;

	/* Allocate memory for window (buffer). */
	msg *window = malloc(window_size * sizeof(msg));
	if (window == NULL) {
		close(file_descriptor);
		return EXIT_FAILURE;
	}

	init(HOST, PORT);

	/* Complete the first message - filename. */
	win_packet.type     = TYPE_FILENAME;
	win_packet.flags    = 0;
	win_packet.checksum = 0;
	win_packet.seq      = 0;
	win_packet.wsize    = window_size;
	strcpy(win_packet.data, argv[1]);

	/* Update the length of the message and copy data. */
	window[0].len = PACKET_HEAD + strlen(argv[1]);
	memcpy(window[0].payload, &win_packet, window[0].len);

	/* Compute the CRC and update the header in message. */
	win_packet.checksum = compute_checksum(&(window[0]));
	memcpy(window[0].payload, &win_packet, PACKET_HEAD);

	/* Send the filename. Be sure to send, so wait for ACK. */
	for (;;) {
		while (send_message(&(window[0])) != (int) sizeof(window[0])) {
		}

		/* If no response arrived, send again the filename. Hellooo! */
		if (recv_message_timeout(&msg_tmp, timeout) != -1) {
			memcpy(&win_packet, msg_tmp.payload, PACKET_HEAD);

			/* ACK. It is there, so do not send again. */
			if (win_packet.type == TYPE_ACK)
				break;
		}
	}

	/* Start to send data from file. */
	for (;;) {
		if (!file_end_found && last_sent - last_confirmed < window_size) {
			/* Free space in window, make a new packet and send it. */
			win_index = (++last_sent) % window_size;

			/* Read directly in a window cell. Verify br (bytes read) value.*/
			int br = read(file_descriptor,
			              window[win_index].payload + PACKET_HEAD, PACKET_DATA);

			/* Update the header of the new packet. */
			if (br <= 0) {
				win_packet.type = TYPE_END;
				file_end_found  = 1;

			} else
				win_packet.type = TYPE_DATA;

			win_packet.flags    = 0;
			win_packet.checksum = 0;
			win_packet.seq      = last_sent;

			/* Copy directly in a window cell in buffer. */
			memcpy(window[win_index].payload, &win_packet, PACKET_HEAD);

			/* Update the length of the message. */
			window[win_index].len = PACKET_HEAD + br;

			/* Compute the checksum and update the header. */
			win_packet.checksum = compute_checksum(&(window[win_index]));
			memcpy(window[win_index].payload, &win_packet, PACKET_HEAD);

			/* Send it. Be sure that all packet is sent. */
			while (send_message(&(window[win_index])) !=
			       (int) sizeof(window[win_index])) {
			}

		} else if (nack_found) {
			win_index  = nack_found % window_size;
			nack_found = 0;

			/* Send again the packet because I received NACK. */
			while (send_message(&(window[win_index])) !=
			       (int) sizeof(window[win_index])) {
			}
		} else {
			/* Buffer full or EOF was found. Wait ACKs.
			 * Get the index of the last packet unconfirmed. */
			win_index = (last_confirmed + 1) % window_size;

			if (recv_message_timeout(&msg_tmp, timeout) == -1) {
				/* If no response arrived, send again the last unconfirmed.
				 */
				while (send_message(&(window[win_index])) !=
				       (int) sizeof(window[win_index])) {
				}

			} else {
				/* A packet from receiver arrived. Guess: ACK or NACK? */
				memcpy(&win_packet, msg_tmp.payload, PACKET_HEAD);

				switch (win_packet.type) {
				case TYPE_ACK:
					/* Sure that receiver has all packets until this seq. */
					last_confirmed = win_packet.seq;

					if (file_end_found && last_confirmed + 1 >= last_sent) {
						/* Close the communication. Goodbye. */
						free(window);
						close(file_descriptor);
						return EXIT_SUCCESS;
					}

					break;

				case TYPE_NACK:
					/* Mark NACK and store the seq in this variable. */
					nack_found = win_packet.seq;
				}
			}
		}
	}

	/* Clean up. */
	if (window != NULL)
		free(window);

	if (file_descriptor != -1)
		close(file_descriptor);

	return EXIT_SUCCESS;
}
