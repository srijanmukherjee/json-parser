CFLAGS=-Wall -Werror

main: libjson.a main.c
	cc $(CFLAGS) -o main main.c -ljson -L. 

libjson.a: json.o lexer.o
	ar rcs libjson.a lexer.o json.o 

json.o: json.h json.c
	cc $(CFLAGS) -c -o json.o json.c

lexer.o: lexer.h lexer.c
	cc $(CFLAGS) -c -o lexer.o lexer.c

clean:
	rm main *.o *.a