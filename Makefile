# Copyright 2019, Petr Vaněk
# SPDX-License-Identifier: MIT

PREFIX ?= /usr

CFLAGS ?= -O2 -pipe
CFLAGS += -Wall -pedantic -Werror=implicit-function-declaration

TARGET = tapcat

.PHONY: all
all: $(TARGET)

.PHONY: install
install: $(TARGET)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin

.PHONY: clean
clean:
	$(RM) -f $(TARGET)
