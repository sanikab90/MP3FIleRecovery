all: main.c
	gcc -w -o a.out main.c
	./a.out /dev/sdc1 recover.mp3
	
clean:
	$(RM) a.out recover.mp3
