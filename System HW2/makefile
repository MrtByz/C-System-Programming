processP: processP.o processC.o
	gcc processP.o -o processP -lm
	gcc processC.o -o processC

processP.o: processP.c
	gcc -c processP.c
	gcc -c processC.c
clean:
	rm *.o processP processC
