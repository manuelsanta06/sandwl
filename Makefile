CC?=gcc
CXX?=g++
VERSION?=0.0.0

PKG_CONFIG ?= pkg-config

PKGS = wlroots-0.20 wayland-server xkbcommon lua54 egl glesv2

CFLAGS_PKG_CONFIG != $(PKG_CONFIG) --cflags $(PKGS)
LIBS != $(PKG_CONFIG) --libs $(PKGS)

WAYLAND_SCANNER = wayland-scanner
WLR_PROTOCOLS = /usr/share/wlr-protocols
LAYER_SHELL_XML = $(WLR_PROTOCOLS)/unstable/wlr-layer-shell-unstable-v1.xml
LAYER_SHELL_HDR = build/wlr-layer-shell-unstable-v1-protocol.h

CFLAGS ?= -O2
CFLAGS += -Wall -Wextra -Isrc -Ibuild -MMD -MP -DVERSION=\"$(VERSION)\" $(CFLAGS_PKG_CONFIG)

CXXFLAGS ?= -O2
CXXFLAGS += -Wall -Wextra -Isrc -Ibuild -MMD -MP -DVERSION=\"$(VERSION)\" $(CFLAGS_PKG_CONFIG) -std=c++23

LDFLAGS ?=


C_SRCS = $(wildcard src/*.c) $(wildcard src/*/*.c)
CXX_SRCS = $(wildcard src/*.cpp) $(wildcard src/*/*.cpp)

C_OBJS = $(patsubst src/%.c,build/%.o,$(C_SRCS))
CXX_OBJS = $(patsubst src/%.cpp,build/%.o,$(CXX_SRCS))
OBJS = $(C_OBJS) $(CXX_OBJS)


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
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS) $(LIBS)

build/%.o: src/%.c $(LAYER_SHELL_HDR) | build
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -DWLR_USE_UNSTABLE -o $@

build/%.o: src/%.cpp | build
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -DWLR_USE_UNSTABLE -o $@

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
