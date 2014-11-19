CC   = gcc -Wall -c
LINK = gcc -o



all: test_echo_server_poll test_echo_server_select



clean:
	rm -f *.o test_hm_pair hm



test_echo_server_poll: test_echo_server.o asyncore_poll.o
	$(LINK) test_echo_server_poll test_echo_server.o asyncore_poll.o

test_echo_server_select: test_echo_server.o  asyncore_select.o
	$(LINK) test_echo_server_select test_echo_server.o asyncore_select.o



test_echo_server.o: test_echo_server.c asyncore.h
	$(CC) test_echo_server.c



asyncore_poll.o: asyncore_poll.c asyncore_static.c asyncore.h
	$(CC) asyncore_poll.c

asyncore_select.o: asyncore_select.c asyncore_static.c asyncore.h
	$(CC) asyncore_select.c

