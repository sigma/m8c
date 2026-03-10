#Set the compiler you are using
CC ?= gcc

#Read version from VERSION file
VERSION := $(shell cat VERSION)

#Set the filename extension of your C files
EXTENSION = .c

# Location of the source files
SOURCE_DIR = src/

# Find all source files in the src directory and subdirectories
SRC_FILES := $(shell find $(SOURCE_DIR) -type f -name "*$(EXTENSION)")

# Find all header files for dependencies
DEPS := $(shell find src -type f -name "*.h")


# SDL version: 2 or 3 (default: 3)
SDL_VERSION ?= 3

# Build directory per SDL version to avoid stale object conflicts
BUILD_DIR = build/sdl$(SDL_VERSION)

# Convert source files to object files in the build directory
OBJ := $(SRC_FILES:%.c=$(BUILD_DIR)/%.o)

ifeq ($(SDL_VERSION),2)
    SDL_PKG = sdl2
    SDL_DEFINE = -DUSE_SDL2
else
    SDL_PKG = sdl3
    SDL_DEFINE =
endif

#Any special libraries you are using in your project (e.g. -lbcm2835 -lrt `pkg-config --libs gtk+-3.0` ), or leave blank
INCLUDES = $(shell pkg-config --libs $(SDL_PKG) libserialport | sed 's/-mwindows//')

# Common compiler flags shared across all build targets
COMMON_CFLAGS = -DAPP_VERSION=\"v$(VERSION)\" -Wall -Wextra -O2 -pipe -I. -DNDEBUG

#Set any compiler flags you want to use (e.g. -I/usr/include/somefolder `pkg-config --cflags gtk+-3.0` ), or leave blank
local_CFLAGS = $(CFLAGS) $(shell pkg-config --cflags $(SDL_PKG) libserialport) -DUSE_LIBSERIALPORT $(SDL_DEFINE) $(COMMON_CFLAGS)

#define a rule that applies to all files ending in the .o suffix, which says that the .o file depends upon the .c version of the file and all the .h files included in the DEPS macro.  Compile each object file
$(BUILD_DIR)/%.o: %$(EXTENSION) $(DEPS)
	@mkdir -p $(dir $@)
	$(CC) -c -o $@ $< $(local_CFLAGS)

#Combine them into the output file
#Set your desired exe output file name here
m8c: $(OBJ)
	$(CC) -o $@ $^ $(local_CFLAGS) $(INCLUDES)

libusb: INCLUDES = $(shell pkg-config --libs $(SDL_PKG) libusb-1.0)
libusb: local_CFLAGS = $(CFLAGS) $(shell pkg-config --cflags $(SDL_PKG) libusb-1.0) $(SDL_DEFINE) -DUSE_LIBUSB=1 $(COMMON_CFLAGS)
libusb: m8c

rtmidi: INCLUDES = $(shell pkg-config --libs $(SDL_PKG) rtmidi)
rtmidi: local_CFLAGS = $(CFLAGS) $(shell pkg-config --cflags $(SDL_PKG) rtmidi) $(SDL_DEFINE) -DUSE_RTMIDI $(COMMON_CFLAGS)
rtmidi: m8c

#Cleanup
.PHONY: clean

clean:
	rm -rf build *~ m8c

# PREFIX is environment variable, but if it is not set, then set default value
ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

install: m8c
	install -d $(DESTDIR)$(PREFIX)/bin/
	install -m 755 m8c $(DESTDIR)$(PREFIX)/bin/
