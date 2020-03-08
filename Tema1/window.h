#ifndef WINDOW_H
#define WINDOW_H

#define PACKET_HEAD 24
#define PACKET_DATA 1376

#define TIMEOUT 5

#define TYPE_ACK 'A'
#define TYPE_NACK 'N'
#define TYPE_DATA 'D'
#define TYPE_END 'E'
#define TYPE_FILENAME 'F'

/* The structure that will be on payload. */
typedef struct {
	char type;
	char flags;

	int checksum;

	unsigned long seq;   /* Sequence number. */
	unsigned long wsize; /* Window size. */

	char data[PACKET_DATA];
} window_packet;

int compute_checksum(msg *message)
{
	int result = 0, byte;

	for (byte = 0; byte < message->len; byte++) {
		result ^= message->payload[byte];
	}

	return result;
}

#endif
