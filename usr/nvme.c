#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include "nvme.h"
#include "nvme_ioctl.h"

int open_dev(const char *dev)
{
	int err = -1;
	struct stat nvme_stat;

	int nvme_fd = open(dev, O_RDONLY);
	if (nvme_fd < 0)
		goto perror;

	err = fstat(nvme_fd, &nvme_stat);
	if (err < 0)
		goto perror;

	if (!S_ISCHR(nvme_stat.st_mode) && !S_ISBLK(nvme_stat.st_mode)) {
		fprintf(stderr, "%s is not a block or character device\n", dev);
		return -1;
	}

	return nvme_fd;
 perror:
	perror(dev);
	return -1;
}

int nvme_obj_read(int nvme_fd, uint64_t key, void **buffer, uint32_t *length) {
	if (!buffer || !length) {
		printf("invalid null ptr.\n");
		return -1;
	}

	struct nvme_user_obj_io io = {0};
	io.opcode = nvme_kv_retrieve;
	io.offset = 0;
	io.length = 1;
	io.key_low = key;
	io.key_high = 0;
	io.key_len = 16;

	if (posix_memalign(buffer, getpagesize(), 1024)) {
		printf("failed to allocate memory nvme read request.\n");
		return -1;
	}

	int ret = nvme_obj_io(nvme_fd, &io, length, buffer);
	if (ret) {
		printf("failed to execute nvme io for reading.\n");
		free(buffer);
		return -1;
	}

	return 0;
}

int nvme_obj_write(int nvme_fd, uint64_t key, const void *buffer, uint32_t length) {
	if (length && !buffer) {
		printf("invalid null ptr.\n");
		return -1;
	}

	struct nvme_user_obj_io io = {0};
	io.opcode = nvme_kv_store;
	io.offset = 0;
	io.length = length;
	io.key_low = key;
	io.key_high = 0;
	io.key_len = 16;

	void *data = NULL;
	if (length && posix_memalign(&data, getpagesize(), length)) {
		printf("failed to allocate memory nvme read request.\n");
		return -1;
	}

	if (length)	memcpy(data, buffer, length);

	int ret = nvme_obj_io(nvme_fd, &io, &length, &data);
	free(data);
	if (ret) {
		printf("failed to execute nvme io for writing.\n");
		return -1;
	}

	return 0;
}

int nvme_obj_create(int nvme_fd, uint64_t key) {
	return nvme_obj_write(nvme_fd, key, NULL, 0);
}
