CFLAGS=-Wall -Wextra -Werror -pedantic-errors -ansi
CFLAGS_DEBUG=${CFLAGS} -g -O0
CFLAGS_RELEASE=${CFLAGS} -O2

release: src/main.c clean mkBinDir
	gcc ${CFLAGS_RELEASE} -o bin/network_monitor $<

debug: src/main.c clean mkBinDir
	gcc ${CFLAGS_DEBUG} -o bin/network_monitor $<

mkBinDir:
	mkdir -p bin

clean:
	rm -rf bin
