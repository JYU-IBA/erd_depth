CC = gcc
CFLAGS = -g -Wall # -DDEBUG
CFLAGS += `pkg-config --cflags jibal`

LIB=`pkg-config --libs jibal`

LDFLAGS = -g

OBJS = erd_depth.o

PROG = erd_depth

$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) -o $(PROG) $(OBJS) $(LIB)

clean:
	rm -f $(OBJS) $(PROG) .depend

depend .depend:
	$(CC) $(CFLAGS) -MM *.c > .depend
include .depend
