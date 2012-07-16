all:
	gcc -o wserv src/*.c -Wall -Werror
clean:
	rm wserv
