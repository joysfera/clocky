#
# Makefile fÅr das Demo-Program der CF-Lib fÅr GNU C
#
CC = gcc
CFLAGS = -Wall -O2 -g
LDFLAGS = 
LIBS = -lcflib -lgemx -lgem
DEFS = 

PROG = setclock.app

SRCS = setclock.c
OBJS = $(subst .c,.o,$(SRCS))

all: $(PROG)

$(PROG): $(OBJS) setclock.h /usr/local/lib/libcflib.a
	$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o $@

.c.o: $(SRCS) setclock.h /usr/local/include/cflib.h
	$(CC) $(CFLAGS) $(DEFS) -c $<

clean:
	rm -f *.o

