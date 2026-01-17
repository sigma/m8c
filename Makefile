#Set the compiler you are using
CC ?= gcc

#Set the filename extension of your C files
EXTENSION = .c

# Location of the source files
SOURCE_DIR = src/

# Find all source files in the src directory and subdirectories
SRC_FILES := $(shell find $(SOURCE_DIR) -type f -name "*$(EXTENSION)")

# Convert to object files
OBJ := $(SRC_FILES:.c=.o)

# Find all header files for dependencies
DEPS := $(shell find src -type f -name "*.h")


# SDL version: 2 or 3 (default: 3)
SDL_VERSION ?= 3

ifeq ($(SDL_VERSION),2)
    SDL_PKG = sdl2
    SDL_DEFINE = -DUSE_SDL2
else
    SDL_PKG = sdl3
    SDL_DEFINE =
endif

#Any special libraries you are using in your project (e.g. -lbcm2835 -lrt `pkg-config --libs gtk+-3.0` ), or leave blank
INCLUDES = $(shell pkg-config --libs $(SDL_PKG) libserialport | sed 's/-mwindows//')

#Set any compiler flags you want to use (e.g. -I/usr/include/somefolder `pkg-config --cflags gtk+-3.0` ), or leave blank
local_CFLAGS = $(CFLAGS) $(shell pkg-config --cflags $(SDL_PKG) libserialport) -DUSE_LIBSERIALPORT $(SDL_DEFINE) -Wall -Wextra -O2 -pipe -I. -DNDEBUG

#define a rule that applies to all files ending in the .o suffix, which says that the .o file depends upon the .c version of the file and all the .h files included in the DEPS macro.  Compile each object file
%.o: %$(EXTENSION) $(DEPS)
	$(CC) -c -o $@ $< $(local_CFLAGS)

#Combine them into the output file
#Set your desired exe output file name here
m8c: $(OBJ)
	$(CC) -o $@ $^ $(local_CFLAGS) $(INCLUDES)

libusb: INCLUDES = $(shell pkg-config --libs $(SDL_PKG) libusb-1.0)
libusb: local_CFLAGS = $(CFLAGS) $(shell pkg-config --cflags $(SDL_PKG) libusb-1.0) $(SDL_DEFINE) -Wall -Wextra -O2 -pipe -I. -DUSE_LIBUSB=1 -DNDEBUG
libusb: m8c

rtmidi: INCLUDES = $(shell pkg-config --libs $(SDL_PKG) rtmidi)
rtmidi: local_CFLAGS = $(CFLAGS) $(shell pkg-config --cflags $(SDL_PKG) rtmidi) $(SDL_DEFINE) -Wall -Wextra -O2 -pipe -I. -DUSE_RTMIDI -DNDEBUG
rtmidi: m8c

#Cleanup
.PHONY: clean

clean:
	rm -f src/*.o src/backends/*.o *~ m8c

# PREFIX is environment variable, but if it is not set, then set default value
ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

install: m8c
	install -d $(DESTDIR)$(PREFIX)/bin/
	install -m 755 m8c $(DESTDIR)$(PREFIX)/bin/
