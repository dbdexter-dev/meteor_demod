SRC=$(wildcard *.c)
OBJ=${SRC:.c=.o}

LDFLAGS += -lm 

.PHONY: strip clean

default: meteor_demod

strip: meteor_demod
	strip $^

meteor_demod: ${OBJ}
	gcc ${LDFLAGS} -o $@ $^

ui.o: ui.c
	gcc -c -o $@ $<

%.o: %.c
	gcc ${CFLAGS} -c -o $@ $<

clean:
	rm -f ${OBJ} meteor_demod
