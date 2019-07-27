/* Copyright 2019, Petr Vaněk */
/* SPDX-License-Identifier: MIT */

#include <linux/if.h>
#include <linux/if_tun.h>

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define TUN_DEV "/dev/net/tun"

#define LEN(x) (sizeof x / sizeof * x)

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
		dup2(tap_fd, 0);
		dup2(tap_fd, 1);
		ret = execvp(*argv, argv);
		if (ret == -1) {
			perror("exec");
			return 1;
		}
	}

	memset(pfds, 0, sizeof pfds);

	pfds[0].fd = 0;
	pfds[0].events = POLLIN | POLLHUP;

	pfds[1].fd = tap_fd;
	pfds[1].events = POLLIN | POLLHUP;

	for (;;) {
		ret = poll(pfds, LEN(pfds), -1);
		if (ret < 0) {
			perror("poll");
			break;
		}

		if (pfds[0].revents & POLLERR || pfds[1].revents & POLLERR) {
			fprintf(stderr, "pollerror\n");
			break;
		}

		if (pfds[0].revents & POLLIN) {
			ret = read(pfds[0].fd, buf, sizeof buf);
			if (ret == 0)
				break;
			all_write(tap_fd, buf, ret);
		}

		if (pfds[1].revents & POLLIN) {
			ret = read(pfds[1].fd, buf, sizeof buf);
			if (ret == 0)
				break;
			all_write(1, buf, ret);
		}

		if (pfds[0].revents & POLLHUP || pfds[1].revents & POLLHUP) {
			break;
		}
	}

	close(tap_fd);

	return 0;
}
