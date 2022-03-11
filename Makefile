CFLAGS=-g -c -Wall -O2
LDFLAGS=-g
SOURCES=sertest.c
OBJECTS=$(SOURCES:.c=.o)

EXECUTABLE=sertest

all: $(SOURCES) $(EXECUTABLE)
	
clean:
	rm *.o
	rm $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) $< -o $@
