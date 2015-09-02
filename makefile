CFLAGS += -g

ep1irc: ep1irc.c clients.o clients.h

clients.o: clients.c clients.h

clean:
	rm *.o *~
