ifdef CROSS_COMPILE
CC = $(CROSS_COMPILE)
endif

SRC := aesdsocket.c
TARGET = aesdsocket
OBJS := $(SRC:.c=.o)

ifdef DEBUG
CFLAGS += -ggdb
endif

all: $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $(OBJS) -o $(TARGET) $(LDFLAGS)

clean:
	-rm -f *.o $(TARGET) *.elf *.map