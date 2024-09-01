GCC = gcc
CFLAGS = -Wall -Wshadow -pthread -g -lrt # para manipulacion de memoria compartida

OBJS = pipeline.o bmp.o publicador.o desenfocador.o realzador.o combinador.o

pipeline: $(OBJS)
	$(GCC) $(CFLAGS) $(OBJS) -o $@

%.o: %.c
	$(GCC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) pipeline
