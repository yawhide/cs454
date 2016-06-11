all:
	@gcc -o stringServer server.c
	@gcc -o stringClient client.c

test:
	@make all
	@SERVER_ADDRESS=127.0.0.1 SERVER_PORT=8000 ./stringServer
