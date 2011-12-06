all: reader

reader: main.c
	cc -o reader -g main.c

release:
	cc -o reader main.c
