all:
	gcc -g -std=c99 -Wall -Wconversion -Wno-sign-conversion -Werror -o pruebas *.c
mem:
	valgrind --leak-check=full --track-origins=yes --show-reachable=yes ./pruebas
clean:
	rm *.o pruebas
zip:
	zip pila *.c *.h