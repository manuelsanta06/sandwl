CC ?= gcc
VERSION ?= 0.0.0

PKG_CONFIG ?= pkg-config

PKGS = wlroots-0.20 wayland-server xkbcommon lua54

CFLAGS_PKG_CONFIG != $(PKG_CONFIG) --cflags $(PKGS)
LIBS != $(PKG_CONFIG) --libs $(PKGS)

WAYLAND_SCANNER = wayland-scanner
WLR_PROTOCOLS = /usr/share/wlr-protocols
LAYER_SHELL_XML = $(WLR_PROTOCOLS)/unstable/wlr-layer-shell-unstable-v1.xml
LAYER_SHELL_HDR = build/wlr-layer-shell-unstable-v1-protocol.h

CFLAGS ?= -O2
CFLAGS += -Wall -Wextra -Isrc -Ibuild -MMD -MP -DVERSION=\"$(VERSION)\" $(CFLAGS_PKG_CONFIG)
LDFLAGS ?=

SRCS = $(wildcard src/*.c) $(wildcard src/*/*.c)
OBJS = $(patsubst src/%.c,build/%.o,$(SRCS))

DEPS = $(OBJS:.o=.d)

TARGET_NAME = compositor
TARGET = build/$(TARGET_NAME)

PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin

all: $(TARGET)

build:
	@mkdir -p build

$(LAYER_SHELL_HDR): | build
	$(WAYLAND_SCANNER) server-header $(LAYER_SHELL_XML) $@

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS) $(LIBS)

build/%.o: src/%.c $(LAYER_SHELL_HDR) | build
	@mkdir -p $(dir $@)
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
