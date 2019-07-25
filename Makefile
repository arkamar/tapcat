# Copyright 2019, Petr Vaněk
# SPDX-License-Identifier: MIT

CFLAGS ?= -O2 -pipe
CFLAGS += -Wall -pedantic -Werror=implicit-function-declaration

all: tapcat
