CC   = gcc -Wall -c
LINK = gcc -o



all: test_echo_server_poll



clean:
	rm -f *.o test_hm_pair hm



test_echo_server_poll: test_echo_server.o asyncore_poll.o
	$(LINK) test_echo_server_poll test_echo_server.o asyncore_poll.o



test_echo_server.o: test_echo_server.c asyncore.h
	$(CC) test_echo_server.c



asyncore_poll.o: asyncore_poll.c asyncore.h
	$(CC) asyncore_poll.c

