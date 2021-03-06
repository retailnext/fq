#include <ck_ring.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../common.h"

#ifndef ITERATIONS
#define ITERATIONS (128000)
#endif

struct entry {
	int tid;
	int value;
};
CK_RING(entry, entry_ring)
static CK_RING_INSTANCE(entry_ring) ring;

int
main(int argc, char *argv[])
{
	int i, r, size;
	uint64_t s, e, e_a, d_a;
	struct entry *buffer;
	struct entry entry = {0, 0};

	if (argc != 2) {
		fprintf(stderr, "Usage: latency <size>\n");
		exit(EXIT_FAILURE);
	}

	size = atoi(argv[1]);
	if (size <= 4 || (size & (size - 1))) {
		fprintf(stderr, "ERROR: Size must be a power of 2 greater than 4.\n");
		exit(EXIT_FAILURE);
	}

	buffer = malloc(sizeof(struct entry) * size);
	if (buffer == NULL) {
		fprintf(stderr, "ERROR: Failed to allocate buffer\n");
		exit(EXIT_FAILURE);
	}

	CK_RING_INIT(entry_ring, &ring, buffer, size);

	e_a = d_a = s = e = 0;
	for (r = 0; r < ITERATIONS; r++) {
		for (i = 0; i < size / 4; i += 4) {
			s = rdtsc();
			CK_RING_ENQUEUE_SPSC(entry_ring, &ring, &entry);
			CK_RING_ENQUEUE_SPSC(entry_ring, &ring, &entry);
			CK_RING_ENQUEUE_SPSC(entry_ring, &ring, &entry);
			CK_RING_ENQUEUE_SPSC(entry_ring, &ring, &entry);
			e = rdtsc();
		}
		e_a += (e - s) / 4;

		for (i = 0; i < size / 4; i += 4) {
			s = rdtsc();
			CK_RING_DEQUEUE_SPSC(entry_ring, &ring, &entry);
			CK_RING_DEQUEUE_SPSC(entry_ring, &ring, &entry);
			CK_RING_DEQUEUE_SPSC(entry_ring, &ring, &entry);
			CK_RING_DEQUEUE_SPSC(entry_ring, &ring, &entry);
			e = rdtsc();
		}
		d_a += (e - s) / 4;
	}

	printf("%10d %16" PRIu64 " %16" PRIu64 "\n", size, e_a / ITERATIONS, d_a / ITERATIONS);
	return (0);
}
