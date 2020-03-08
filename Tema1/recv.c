#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "link_emulator/lib.h"
#include "window.h"

#define HOST "127.0.0.1"
#define PORT 10001

int main(int argc, char **argv)
{
	int file_descriptor = -1;
	int received_checksum;

	unsigned long windex         = 0;
	unsigned long window_size    = 0;
	unsigned long last_confirmed = -1;

	msg msg_tmp;
	window_packet wpacket;

	msg *window    = NULL;
	char *filename = NULL;

	/* Check the number of arguments. */
	if (argc > 1 || argv == NULL)
		return EXIT_FAILURE;

	init(HOST, PORT);

	/* Start the process. */
	for (;;) {
		/* Wait for a new message. Blocked until something comes. */
		if (recv_message(&msg_tmp) < 0) {
			/* Error! Send NACK for the expected packet and wait again. */
			wpacket.type = TYPE_NACK;
			wpacket.seq  = last_confirmed + 1;

			memcpy(msg_tmp.payload, &wpacket, PACKET_HEAD);
			while (send_message(&msg_tmp) != (int) sizeof(msg_tmp)) {
			}

			/* Loop restart. */
			continue;
		}

		/* Get the header from the new message. */
		memcpy(&wpacket, msg_tmp.payload, PACKET_HEAD);

		/* Already received and confirmed this packet. */
		if (wpacket.seq < last_confirmed + 1)
			/* Loop restart. */
			continue;

		/* Replace checksum value with 0, compute it again and compare. */
		received_checksum = wpacket.checksum;

		wpacket.checksum = 0;
		memcpy(msg_tmp.payload, &wpacket, PACKET_HEAD);

		if (received_checksum != compute_checksum(&msg_tmp)) {
			/* Send NACK for this corrupt packet. */
			wpacket.type = TYPE_NACK;

			memcpy(msg_tmp.payload, &wpacket, PACKET_HEAD);
			while (send_message(&msg_tmp) != (int) sizeof(msg_tmp)) {
			}

			/* Loop restart. Wait again... */
			continue;
		}

		/* This message is the expected one. Write in file or create file.  */
		if (wpacket.seq == last_confirmed + 1) {
			last_confirmed++;

			switch (wpacket.type) {
			case TYPE_FILENAME:
				/* Create the correct filename and open the file. */
				filename = malloc(msg_tmp.len - PACKET_HEAD + 6);
				sprintf(filename, "recv_%s", msg_tmp.payload + PACKET_HEAD);

				/* Open the file and verify the result. */
				file_descriptor =
				    open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
				free(filename);

				if (file_descriptor == -1)
					return EXIT_FAILURE;

				/* Set the same window size on the receiver. */
				window_size = wpacket.wsize;

				/* Allocate memory for window (buffer). */
				window = calloc(window_size, sizeof(msg));
				if (window == NULL) {
					close(file_descriptor);
					return EXIT_FAILURE;
				}

				break;

			case TYPE_DATA:
				/* Write the information directly in file. */
				if (write(file_descriptor, msg_tmp.payload + PACKET_HEAD,
				          msg_tmp.len - PACKET_HEAD) < 0) {
					free(window);
					close(file_descriptor);
					return EXIT_FAILURE;
				}

				break;

			case TYPE_END:
				/* Close the communication. Au revoir. */
				free(window);
				close(file_descriptor);
				return EXIT_SUCCESS;
			}

			/* Check if the next expected packet(s) is(are) in buffer. */
			for (windex = (last_confirmed + 1) % window_size;
			     window[windex].len != 0; windex = (windex + 1) % window_size) {
				/* If length of the packet != 0 means that it is valid. */
				last_confirmed++;

				if (write(file_descriptor, window[windex].payload + PACKET_HEAD,
				          window[windex].len - PACKET_HEAD) < 0) {
					free(window);
					close(file_descriptor);
					return EXIT_FAILURE;
				}

				/* Invalidate this packet. */
				window[windex].len = 0;
			}

			/* Send ACK. */
			wpacket.type = TYPE_ACK;
			wpacket.seq  = last_confirmed;
			memcpy(msg_tmp.payload, &wpacket, PACKET_HEAD);
			while (send_message(&msg_tmp) != (int) sizeof(msg_tmp)) {
			}

		} else {
			/* Another packet arrived first. */
			windex = wpacket.seq % window_size;

			if (window[windex].len != 0) {
				/*  Already occupied this place. So go to wait, wait, wait. */
				continue;
			}

			window[windex].len = msg_tmp.len;
			memcpy(window[windex].payload, msg_tmp.payload, msg_tmp.len);
		}
	}

	/* Clean up. */
	if (window != NULL)
		free(window);

	if (file_descriptor != -1)
		close(file_descriptor);

	return EXIT_SUCCESS;
}
