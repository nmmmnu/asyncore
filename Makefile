CC   = gcc -Wall -c
LINK = gcc -o



all: test_echo_server_poll test_echo_server_select test_sb



clean:
	rm -f *.o test_echo_server_poll test_echo_server_select test_sb



asyncore.o: asyncore.c asyncore.h asyncore_method.h
	$(CC) asyncore.c



asyncore_poll.o: asyncore_poll.c asyncore.h asyncore_method.h
	$(CC) asyncore_poll.c

asyncore_select.o: asyncore_select.c asyncore.h asyncore_method.h
	$(CC) asyncore_select.c



sb.o: sb.c sb.h
	$(CC) sb.c



# ===========================


test_echo_server_poll: test_echo_server.o asyncore.o asyncore_poll.o
	$(LINK) test_echo_server_poll test_echo_server.o asyncore.o asyncore_poll.o

test_echo_server_select: test_echo_server.o asyncore.o asyncore_select.o
	$(LINK) test_echo_server_select test_echo_server.o asyncore.o asyncore_select.o

test_echo_server.o: test_echo_server.c asyncore.h
	$(CC) test_echo_server.c



test_sb.o: test_sb.c sb.h
	$(CC) test_sb.c

test_sb: sb.o test_sb.o
	$(LINK) test_sb sb.o test_sb.o
