#include <errno.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


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

int nvme_read_data(int nvme_fd, uint64_t key, void **buffer, uint32_t length, int is_attrib) 
{
	if (!buffer || !length) {
		printf("invalid null ptr.\n");
		return -1;
	}

	struct nvme_user_obj_io io = {
        .opcode = nvme_kv_retrieve,
        .offset = 0,
        .length = length,
        .key_low = key,
        .key_high = 0,
        .key_len = NVME_OBJ_ID_MAXLEN
    };

	if (posix_memalign(buffer, getpagesize(), length)) {
		printf("failed to allocate memory nvme read request.\n");
		return -1;
	}

	int ret = nvme_obj_io(nvme_fd, &io, &length, buffer, is_attrib);
	if (ret) {
		printf("failed to execute nvme io for reading.\n");
		free(buffer);
		return -1;
	}

	return 0;
}

int nvme_write_data(int nvme_fd, uint64_t key, const void *buffer, uint32_t length, bool is_attrib) 
{
	if (length && !buffer) {
		printf("invalid null ptr.\n");
		return -1;
	}

	struct nvme_user_obj_io io = {
        .opcode = nvme_kv_store,
        .offset = 0,
        .length = length,
        .key_low = key,
        .key_high = 0,
        .key_len = NVME_OBJ_ID_MAXLEN
    };

	void *data = NULL;
	if (length && posix_memalign(&data, getpagesize(), length)) {
		printf("failed to allocate memory nvme read request.\n");
		return -1;
	}

	if (length)
        memcpy(data, buffer, length);

	int ret = nvme_obj_io(nvme_fd, &io, &length, &data, is_attrib);
	free(data);
	if (ret) {
		printf("failed to execute nvme io for writing.\n");
		return -1;
	}

	return 0;
}

int nvme_obj_create(int nvme_fd, uint64_t key) {
    int ret;
    void* buf = kzalloc(getpagesize(), GFP_KERNEL);
    if(!buf)
        return -ENOMEM;

	ret = nvme_attribute_write(nvme_fd, key, buf, getpagesize());
    free(buf);

    return ret;
}

int nvme_obj_read(int nvme_fd, uint64_t key, void **buffer, uint32_t length)
{
    nvme_read_data(nvme_fd, key, buffer, length, false);
}

int nvme_obj_write(int nvme_fd, uint64_t key, const void *buffer, uint32_t length)
{
    nvme_write_data(nvme_fd, key, buffer, length, false);
}

int nvme_attribute_read(int nvme_fd, uint64_t key, void **buffer, uint32_t length)
{
    nvme_read_data(nvme_fd, key, buffer, length, true);
}

int nvme_attribute_write(int nvme_fd, uint64_t key, const void *buffer, uint32_t length)
{
    nvme_write_data(nvme_fd, key, buffer, length, true);
}
