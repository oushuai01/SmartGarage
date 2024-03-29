SRC := $(wildcard *.c)
ELF := $(SRC:%.c=%)

ELF := $(filter-out camera, $(ELF))
ELF := $(filter-out Video, $(ELF))

all:$(ELF) Video

CROSS = arm-none-linux-gnueabi-
CC = $(CROSS)gcc

CFLAGS  += \
	-I include/

LDFLAGS += \
	-L lib \
	-lsqlite3arm \
	-lpthread \
	-ldl \
	-ljpeg \
	-Wl,-rpath=./lib

$(ELF):%:%.c
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS)

Video:Video.c camera.c
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS)

clean:
	$(RM) $(ELF) Video
