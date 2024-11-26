CC := gcc
CFLAGS := -Wall -O2

TARGETS := server client

all: $(TARGETS)

%: %.cpp
	$(CC) -o $@ $< $(CFLAGS)

clean:
	rm -f $(TARGETS)

.PHONY: all clean
