all: myfilesystem

myfilesystem: mfs.o login.o commands.o filesystem.o
	gcc -o myfilesystem mfs.o login.o commands.o filesystem.o -lm

mfs.o: mfs.c
	gcc -Wall -c mfs.c

login.o: login.c
	gcc -Wall -c login.c

commands.o: commands.c
	gcc -Wall -c commands.c

filesystem.o: filesystem.c
	gcc -Wall -c filesystem.c

clean:
	rm -f login.o mfs.o commands.o filesystem.o