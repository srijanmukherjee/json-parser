main: json.h main.c
	cc -Wall -Werror -o main main.c

clean:
	rm main