CC = cc
CFLAGS = -Wall -Wextra -lm -ldl
TARGET = main list.so degree2.so

build: ${TARGET}

main: main.c
	${CC} ${CFLAGS} -o $@ $<

degree2.so: degree2.c
	${CC} ${CFLAGS} -shared -fPIC -o $@ $<

list.so: list_allocator.c
	${CC} ${CFLAGS} -shared -fPIC -o $@ $<


clean:
	@rm -f ${TARGET}
	@rm -rf ./*.dSYM
