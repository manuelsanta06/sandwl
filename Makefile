CC?=gcc
CXX?=g++
VERSION?=0.0.0
RUN_BACKEND ?= wayland
RUN_ARGS ?=

PKG_CONFIG ?= pkg-config
MODE ?= release

ifeq ($(MODE),debug)
BUILD_DIR = build/debug
MODE_FLAGS = -O0 -g3 -DDEBUG
else
BUILD_DIR = build
MODE_FLAGS = -O2
endif

PKGS = wlroots-0.20 wayland-server xkbcommon lua54 egl glesv2 assimp

CFLAGS_PKG_CONFIG != $(PKG_CONFIG) --cflags $(PKGS)
LIBS != $(PKG_CONFIG) --libs $(PKGS)

WAYLAND_SCANNER = wayland-scanner
WLR_PROTOCOLS = /usr/share/wlr-protocols
LAYER_SHELL_XML = $(WLR_PROTOCOLS)/unstable/wlr-layer-shell-unstable-v1.xml
LAYER_SHELL_HDR = $(BUILD_DIR)/wlr-layer-shell-unstable-v1-protocol.h

CFLAGS ?=
CFLAGS += $(MODE_FLAGS) -Wall -Wextra -Isrc -I$(BUILD_DIR) -MMD -MP -DVERSION=\"$(VERSION)\" $(CFLAGS_PKG_CONFIG)

CXXFLAGS ?=
CXXFLAGS += $(MODE_FLAGS) -Wall -Wextra -Isrc -I$(BUILD_DIR) -MMD -MP -DVERSION=\"$(VERSION)\" $(CFLAGS_PKG_CONFIG) -std=c++23

LDFLAGS ?=


C_SRCS = $(wildcard src/*.c) $(wildcard src/*/*.c)
CXX_SRCS = $(wildcard src/*.cpp) $(wildcard src/*/*.cpp)

C_OBJS = $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(C_SRCS))
CXX_OBJS = $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(CXX_SRCS))
OBJS = $(C_OBJS) $(CXX_OBJS)


DEPS = $(OBJS:.o=.d)

TARGET_NAME = compositor
TARGET = $(BUILD_DIR)/$(TARGET_NAME)

PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin

all: $(TARGET)

debug:
	$(MAKE) MODE=debug all

run: debug
	WLR_BACKENDS=$(RUN_BACKEND) ./build/debug/$(TARGET_NAME) $(RUN_ARGS)

$(BUILD_DIR)/tests/cameraMode: tests/cameraMode.c $(BUILD_DIR)/cameraMode.o $(BUILD_DIR)/keybindings.o $(BUILD_DIR)/serverKeyboard.o $(BUILD_DIR)/config/luaBindings.o $(BUILD_DIR)/config/luaConfig.o
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -DWLR_USE_UNSTABLE $^ -o $@ $(LDFLAGS) $(LIBS)

$(BUILD_DIR)/tests/cursorMotion: tests/cursorMotion.c $(BUILD_DIR)/cameraMode.o $(BUILD_DIR)/keybindings.o $(BUILD_DIR)/serverCursor.o $(BUILD_DIR)/processCursor.o $(BUILD_DIR)/utilies.o
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -DWLR_USE_UNSTABLE $^ -o $@ $(LDFLAGS) $(LIBS)

test: $(BUILD_DIR)/tests/cameraMode $(BUILD_DIR)/tests/cursorMotion
	./$(BUILD_DIR)/tests/cameraMode
	./$(BUILD_DIR)/tests/cursorMotion

$(BUILD_DIR):
	@mkdir -p $@

$(LAYER_SHELL_HDR): | $(BUILD_DIR)
	$(WAYLAND_SCANNER) server-header $(LAYER_SHELL_XML) $@

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS) $(LIBS)

$(BUILD_DIR)/%.o: src/%.c $(LAYER_SHELL_HDR) | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -DWLR_USE_UNSTABLE -o $@

$(BUILD_DIR)/%.o: src/%.cpp | $(BUILD_DIR)
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

.PHONY: all debug run clean install uninstall test
