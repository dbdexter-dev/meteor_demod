SRC=$(wildcard *.c)
OBJ=${SRC:.c=.o}

CFLAGS += -I./include
LDFLAGS += -lm -lpthread

.PHONY: strip clean

default: meteor_demod

strip: meteor_demod
	strip $^

meteor_demod: ${OBJ}
	gcc ${LDFLAGS} -o $@ $^

main.o: main.c include/options.h
	gcc ${CFLAGS} -c -o $@ $<

%.o: %.c
	gcc ${CFLAGS} -c -o $@ $<

clean:
	rm -f ${OBJ}
