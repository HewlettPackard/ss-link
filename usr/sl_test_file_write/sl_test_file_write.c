// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Hewlett Packard Enterprise Development LP */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_STR_SIZE 32UL /* See CMD_LEN */ 

int main(int argc, char *argv[])
{
	int         rtn;
	int         fd;
	ssize_t     bytes_written;
	size_t      str_len;
	const char *filename;
	const char *str;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s <string> <filename>\n", argv[0]);
		return EINVAL;
	}

	str      = argv[1];
	filename = argv[2];

	str_len = strnlen(str, MAX_STR_SIZE);
	if ((str_len == 0) || (str_len == MAX_STR_SIZE)) {
		fprintf(stderr, "Error: invalid string size (str_len = %lu, MAX_STR_SIZE = %lu)\n", str_len, MAX_STR_SIZE);
		return EINVAL;
	}

	fd = open(filename, O_WRONLY);
	if (fd == -1) {
		rtn = errno;
		perror("open failed");
		return rtn;
	}
	
	rtn = 0;
	bytes_written = write(fd, str, str_len);
	if (bytes_written < 0) {
		rtn = errno;
		perror("write failed");
		goto out;
	}

	if ((size_t)bytes_written != str_len) {
		fprintf(stderr, "Error: bytes_written != str_len (bytes_written = %ld, str_len = %lu)\n", bytes_written, str_len);
		rtn = EIO;
		goto out;
	}

out:
	if (close(fd) == -1)
		perror("close failed");

	return rtn;
}

