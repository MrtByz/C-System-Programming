serverY: serverY.c
	gcc client.c -o client -lrt
	gcc -Wall -pthread serverY.c become_daemon.c -o serverY -lrt
clean:
	rm *.o client serverY
