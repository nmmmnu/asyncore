CC   = gcc -Wall -c
LINK = gcc -o



all: test_echo_server_poll test_echo_server_select test_sb



clean:
	rm -f *.o test_echo_server_poll test_echo_server_select test_sb



asyncore_functions.o: asyncore_functions.c asyncore_functions.h
	$(CC) asyncore_functions.c



asyncore_poll.o: asyncore_poll.c asyncore.h asyncore_functions.h
	$(CC) asyncore_poll.c

asyncore_select.o: asyncore_select.c asyncore.h asyncore_functions.h
	$(CC) asyncore_select.c



sb.o: sb.c sb.h
	$(CC) sb.c



# ===========================


test_echo_server_poll: test_echo_server.o asyncore_poll.o asyncore_functions.o
	$(LINK) test_echo_server_poll test_echo_server.o asyncore_poll.o asyncore_functions.o

test_echo_server_select: test_echo_server.o  asyncore_select.o asyncore_functions.o
	$(LINK) test_echo_server_select test_echo_server.o asyncore_select.o asyncore_functions.o

test_echo_server.o: test_echo_server.c asyncore.h
	$(CC) test_echo_server.c



test_sb.o: test_sb.c sb.h
	$(CC) test_sb.c

test_sb: sb.o test_sb.o
	$(LINK) test_sb sb.o test_sb.o
