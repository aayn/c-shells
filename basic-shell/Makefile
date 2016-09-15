CC=gcc
CFLAGS=-c -g
LDFLAGS=
SOURCES=src/main.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=aaysh

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(OBJECTS) $(EXECUTABLE)
