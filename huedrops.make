HUEDROPS_C_FLAGS=-O2 -Wextra -Waggregate-return -Wcast-align -Wcast-qual -Wconversion -Wformat=2 -Winline -Wlong-long -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -Wno-import -Wpointer-arith -Wredundant-decls -Wreturn-type -Wshadow -Wstrict-prototypes -Wswitch -Wwrite-strings

huedrops: huedrops.o
	gcc -o huedrops huedrops.o

huedrops.o: huedrops.c huedrops.make
	gcc -c ${HUEDROPS_C_FLAGS} -o huedrops.o huedrops.c

clean:
	rm -f huedrops huedrops.o
