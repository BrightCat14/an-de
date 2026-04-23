CC = clang
CFLAGS = -Wall -O2 $(shell pkg-config --cflags x11 xft fontconfig xrandr)
LIBS = $(shell pkg-config --libs x11 xft fontconfig xrandr) -lImlib2 -lXext
TARGET = notande
OBJS = wm.o frames.o panels.o main.o
POWERCTL_DIR = powerctl
POWERCTL_LIB = $(RUST_DIR)/target/release/libpowerctl.so

all: $(TARGET)

$(POWERCTL_LIB):
	cd $(POWERCTL_DIR) && cargo build --release

$(TARGET): $(OBJS) $(POWERCTL_LIB)
	$(CC) $(OBJS) -o $(TARGET) $(LIBS) \
	-L$(POWERCTL_DIR)/target/release -lpowerctl \
	-Wl,-rpath=$(POWERCTL_DIR)/target/release

wm.o: wm.c wm.h
	$(CC) $(CFLAGS) -c wm.c

frames.o: frames.c frames.h panels.h wm.h
	$(CC) $(CFLAGS) -c frames.c

panels.o: panels.c panels.h frames.h wm.h
	$(CC) $(CFLAGS) -c panels.c

main.o: main.c wm.h frames.h panels.h
	$(CC) $(CFLAGS) -c main.c

clean:
	rm -f $(OBJS) $(TARGET)
	cd $(POWERCTL_DIR) && cargo clean

install:
	cp $(TARGET) /usr/local/bin/

run: $(TARGET)
	Xephyr -br -ac -noreset -screen 1280x720 :1 &
	sleep 1
	DISPLAY=:1 ./$(TARGET)

.PHONY: all clean install run
