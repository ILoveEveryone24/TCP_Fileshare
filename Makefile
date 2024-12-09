CC := gcc
CFLAGS := -Wall -O2

TARGETS := server client

all: $(TARGETS)

%: %.c
	$(CC) -o $@ $< $(CFLAGS)

clean:
	rm -f $(TARGETS)

.PHONY: all clean
