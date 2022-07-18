server: server.c
	gcc -Wall -pthread server.c -o server -lrt
	gcc -Wall -pthread servant.c -o servant -lrt
	gcc -Wall -pthread client.c -o client -lrt
clean:
	rm server servant client
