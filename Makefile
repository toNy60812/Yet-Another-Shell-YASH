yash: main.o token.o action.o
	gcc -o yash main.o token.o action.o -lreadline

main.o: main.c
	gcc -c main.c

token.o: token.c
	gcc -c token.c

action.o: action.c
	gcc -c action.c

clean:
	rm -f yash *.o