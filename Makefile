TARGET=ken
CFLAGS=-std=c11 -g -fno-common -Wall

SRCS=main.c lex.c hashmap.c file.c
OBJS=$(SRCS:.c=.o)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: clean
