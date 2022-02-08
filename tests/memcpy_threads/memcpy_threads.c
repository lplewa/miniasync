// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2022, Intel Corporation */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "libminiasync.h"
#include "core/os.h"

enum test_type {SEQUENCE, SINGLE_CHAR};
/*
 * test_threads_memcpy_multiple -- test multiple memcpy operations n times
 * in a loop on a specific descriptor and test type.
 */
int
test_threads_memcpy_multiple(unsigned memcpy_count,
	unsigned n, size_t test_size, struct vdm_descriptor *descriptor,
	enum test_type test_type)
{
	int ret = 0;
	unsigned seed = (unsigned)time(NULL);
	struct runtime *r = runtime_new();
	struct vdm *vdm = vdm_new(descriptor);

	if (vdm == NULL) {
		fprintf(stderr, "Failed to create VDM\n");
		runtime_delete(r);
		return 1;
	}

	char **sources = malloc(memcpy_count * sizeof(char *) * n);
	char **destinations = malloc(memcpy_count * sizeof(char *) * n);
	size_t *sizes = malloc(memcpy_count * sizeof(size_t) * n);

	struct future **futures = malloc(
		memcpy_count * sizeof(struct future *) * n);
	struct vdm_memcpy_future *memcpy_futures =
		malloc(memcpy_count * sizeof(struct vdm_memcpy_future) * n);

	unsigned index = 0;
	char value = 0;
	for (unsigned iter = 0; iter < n; iter++) {
		for (unsigned i = index; i < index + memcpy_count; i++) {
			if (test_size) {
				sizes[i] = test_size;
			} else {
				sizes[i] = (size_t)os_rand_r(&seed)
					% (1 << 20) + 1;
			}
			sources[i] = malloc(sizes[i] * sizeof(char));
			destinations[i] = malloc(sizes[i] * sizeof(char));

			switch (test_type) {
				case SEQUENCE:
					value = 0;
					for (unsigned j = 0;
						j < sizes[i]; j++) {
						sources[i][j] = value++;
					}
					break;
				case SINGLE_CHAR:
					memset(sources[i], value++, sizes[i]);
					break;
				default:
					break;
			}

			memcpy_futures[i] = vdm_memcpy(vdm, destinations[i],
				sources[i], sizes[i], 0);
			futures[i] =
				FUTURE_AS_RUNNABLE(&memcpy_futures[i]);
		}

		runtime_wait_multiple(r, futures + iter * memcpy_count,
			memcpy_count);
		index += memcpy_count;
	}

	/* Verification */
	for (unsigned i = 0; i < memcpy_count * n; i++) {
		if (memcmp(sources[i], destinations[i], sizes[i]) != 0) {
			fprintf(stderr,
				"Memcpy nr. %u result is wrong! "
				"Returning\n", i);

			ret = 1;
			goto cleanup;
		}
		printf("Memcpy nr. %u from [%p] to [%p] n=%lu "
			"content=sequence is correct\n", i, sources[i],
			destinations[i], sizes[i]);
	}

	/* Cleanup */
cleanup:
	for (unsigned i = 0; i < memcpy_count * n; i++) {
		free(sources[i]);
		free(destinations[i]);
	}
	free(sources);
	free(destinations);
	free(sizes);
	free(futures);
	free(memcpy_futures);

	runtime_delete(r);
	vdm_delete(vdm);
	return ret;
}

int
main(int argc, char *argv[])
{
	return
		test_threads_memcpy_multiple(100, 10, 10,
			vdm_descriptor_threads(), SINGLE_CHAR) ||
		test_threads_memcpy_multiple(100, 2, 1 << 10,
			vdm_descriptor_threads(), SINGLE_CHAR) ||
		test_threads_memcpy_multiple(100, 10, 128,
			vdm_descriptor_threads(), SINGLE_CHAR) ||
		test_threads_memcpy_multiple(100, 10, 7,
			vdm_descriptor_threads(), SEQUENCE) ||
		test_threads_memcpy_multiple(100, 1, 1 << 10,
			vdm_descriptor_threads(), SEQUENCE) ||
		test_threads_memcpy_multiple(100, 10, 0,
			vdm_descriptor_threads(), SEQUENCE);
}