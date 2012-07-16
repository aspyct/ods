all:
	gcc -o wserv src/*.c -Wall -Werror
test:
	gcc src/ods_http.c test/ods_http_test.c
	./a.out
clean:
	rm wserv

.PHONY: test clean
