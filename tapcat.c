/* Copyright 2019, Petr Vaněk */
/* SPDX-License-Identifier: MIT */

#include <linux/if.h>
#include <linux/if_tun.h>

#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define TUN_DEV "/dev/net/tun"

#define LEN(x) (sizeof x / sizeof * x)

typedef uint16_t pack_len_t;

struct pack {
	pack_len_t len;
	unsigned char data[];
};

int
all_write(int fd, const unsigned char * buf, int len) {
	register int ret;

	while (len) {
		ret = write(fd, buf, len);
		if (ret == -1) {
			if (errno == EINTR) continue;
			return ret;
		}
		buf += ret;
		len -= ret;
	}
	return 0;
}

int
read_all(int fd, unsigned char * buf, int len) {
	register int ret;

	while (len) {
		ret = read(fd, buf, len);
		if (ret == -1) {
			if (errno == EINTR) continue;
			return ret;
		}
		buf += ret;
		len -= ret;
	}

	return 0;
}

int
open_tap_device(const char * dev) {
	struct ifreq ifr;
	int fd, err;

	fd = open(TUN_DEV, O_RDWR | O_CLOEXEC);
	if (fd < 0) {
		perror("Cannot open " TUN_DEV);
		return fd;
	}

	memset(&ifr, 0, sizeof ifr);

	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
	if (*dev) {
		strncpy(ifr.ifr_name, dev, sizeof ifr.ifr_name - 1);
	}

	if ((err = ioctl(fd, TUNSETIFF, &ifr)) < 0) {
		perror("ioctl");
		close(fd);
		return err;
	}

	return fd;
}

int
main(int argc, char * argv[]) {
	const char * argv0, * name;
	unsigned char buf[0x1000];
	struct pack * pack = (struct pack *)buf;
	struct pollfd pfds[2];
	int tap_fd, ret;

	argv0 = *argv; argv++; argc--;

	if (argc < 1) {
		fprintf(stderr, "%s tap-device [cmd [...]]\n", argv0);
		return 1;
	}

	name = *argv; argv++; argc--;

	tap_fd = open_tap_device(name);
	if (tap_fd == -1) {
		perror("cannot open tap");
		return 1;
	}

	if (*argv) {
		int p[2][2];
		if (pipe(p[0]) == -1 || pipe(p[1]) == -1) {
			perror("pipes");
			return 1;
		}

		int pid = fork();
		if (pid == -1) {
			perror("fork");
			return 1;
		}

		if (pid == 0) {
			dup2(p[0][0], 0);
			dup2(p[1][1], 1);
			close(p[0][0]);
			close(p[1][1]);
			close(p[0][1]);
			close(p[1][0]);
			close(tap_fd);
			ret = execvp(*argv, argv);
			if (ret == -1) {
				perror("exec");
				_exit(1);
			}
		} else {
			dup2(p[0][1], 1);
			dup2(p[1][0], 0);
			close(p[0][0]);
			close(p[1][1]);
			close(p[0][1]);
			close(p[1][0]);
		}
	}

	memset(pfds, 0, sizeof pfds);

	#define STD_IN 0
	#define TAP_FD 1

	pfds[STD_IN].fd = 0;
	pfds[STD_IN].events = POLLIN | POLLHUP;

	pfds[TAP_FD].fd = tap_fd;
	pfds[TAP_FD].events = POLLIN | POLLHUP;

	for (;;) {
		ret = poll(pfds, LEN(pfds), -1);
		if (ret < 0) {
			perror("poll");
			break;
		}

		if (pfds[STD_IN].revents & POLLERR || pfds[TAP_FD].revents & POLLERR) {
			fprintf(stderr, "pollerror\n");
			break;
		}

		if (pfds[STD_IN].revents & POLLIN) {
			ret = read_all(pfds[STD_IN].fd, buf, sizeof pack->len);
			if (ret == -1) {
				fprintf(stderr, "cannot read packet length\n");
				break;
			}

			pack_len_t len = be16toh(pack->len);

			ret = read_all(pfds[STD_IN].fd, pack->data, len);
			if (ret == -1) {
				fprintf(stderr, "cannot read all data\n");
				break;
			}
			all_write(tap_fd, pack->data, len);
		}

		if (pfds[TAP_FD].revents & POLLIN) {
			ret = read(pfds[TAP_FD].fd, pack->data, sizeof buf - sizeof pack->len);
			pack->len = htobe16(ret);
			if (ret == 0)
				break;
			all_write(1, buf, ret + sizeof pack->len);
		}

		if (pfds[STD_IN].revents & POLLHUP || pfds[TAP_FD].revents & POLLHUP) {
			break;
		}
	}

	close(tap_fd);

	return 0;
}
