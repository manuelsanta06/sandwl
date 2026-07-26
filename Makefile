CC ?= gcc
VERSION ?= 0.0.0

PKG_CONFIG ?= pkg-config

PKGS = wlroots-0.20 wayland-server xkbcommon

CFLAGS_PKG_CONFIG != $(PKG_CONFIG) --cflags $(PKGS)
LIBS != $(PKG_CONFIG) --libs $(PKGS)

CFLAGS ?= -O2
CFLAGS += -Wall -Wextra -Isrc -MMD -MP -DVERSION=\"$(VERSION)\" $(CFLAGS_PKG_CONFIG)
LDFLAGS ?=

SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:src/%.c=build/%.o)

DEPS = $(OBJS:.o=.d)

TARGET_NAME = compositor
TARGET = build/$(TARGET_NAME)

PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin

all: $(TARGET)

build:
	@mkdir -p build

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS) $(LIBS)

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c $< -DWLR_USE_UNSTABLE -o $@

clean:
	rm -rf build/

install: all
	@echo "Installing in $(DESTDIR)$(BINDIR)..."
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET_NAME)

uninstall:
	@echo "Uninstalling from $(DESTDIR)$(BINDIR)..."
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET_NAME)

-include $(DEPS)

.PHONY: all clean install uninstall build
