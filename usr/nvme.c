#include <errno.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#include "nvme.h"
#include "nvme_ioctl.h"

#define NVME_ATTR_KEY_HIGH (1ULL << 31)

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

int nvme_read_data(int nvme_fd, uint64_t key, void **buffer, uint32_t length, bool is_attrib) 
{
	uint64_t key_high = 0;

	if (!buffer || !length) {
		printf("invalid null ptr.\n");
		return -1;
	}

	if (is_attrib == true) {
		key_high = NVME_ATTR_KEY_HIGH;
	}

	struct nvme_user_obj_io io = {
        .opcode = nvme_kv_retrieve,
        .offset = 0,
        .length = length,
        .key_low = key,
        .key_high = key_high,
        .key_len = NVME_OBJ_ID_MAXLEN
    };

	if (posix_memalign(buffer, getpagesize(), length)) {
		printf("failed to allocate memory nvme read request.\n");
		return -1;
	}

	int ret = nvme_obj_io(nvme_fd, &io, &length, buffer);
	if (ret) {
		printf("failed to execute nvme io for reading.\n");
		free(*buffer);
		*buffer = NULL;
		return -1;
	}

	return 0;
}

int nvme_write_data(int nvme_fd, uint64_t key, const void *buffer, uint32_t length, bool is_attrib) 
{
    uint64_t key_high = 0;
    
	if (length && !buffer) {
		printf("invalid null ptr.\n");
		return -1;
	}

	if (is_attrib == true) {
		key |= NVME_ATTR_KEY_HIGH;
	}

	struct nvme_user_obj_io io = {
        .opcode = nvme_kv_store,
        .offset = 0,
        .length = length,
        .key_low = key,
        .key_high = 0,
        .key_len = NVME_OBJ_ID_MAXLEN
    };

	printf("\t nvme_write_data: key=%llx, key_high=%llx, length=%d, is_attrib=%d\n", key, key_high, length, is_attrib);

	void *data = NULL;
	if (length && posix_memalign(&data, getpagesize(), length)) {
		printf("failed to allocate memory nvme write request.\n");
		return -1;
	}

	if (length)
        memcpy(data, buffer, length);

	int ret = nvme_obj_io(nvme_fd, &io, &length, &data);
	free(data);
	if (ret) {
		printf("failed to execute nvme io for writing.\n");
		return -1;
	}

	return 0;
}

int nvme_obj_read(int nvme_fd, uint64_t key, void **buffer, uint32_t length)
{
    return nvme_read_data(nvme_fd, key, buffer, length, false);
}

int nvme_obj_write(int nvme_fd, uint64_t key, const void *buffer, uint32_t length)
{
    return nvme_write_data(nvme_fd, key, buffer, length, false);
}

int nvme_attribute_read(int nvme_fd, uint64_t key, void **buffer, uint32_t length)
{
    return nvme_read_data(nvme_fd, key, buffer, length, true);
}

int nvme_attribute_write(int nvme_fd, uint64_t key, const void *buffer, uint32_t length)
{
	printf("\t nvme_attribute_write: key=%llx, length=%d\n", key, length);
    return nvme_write_data(nvme_fd, key, buffer, length, true);
}
