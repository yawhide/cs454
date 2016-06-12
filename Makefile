all:
	@gcc -o stringServer server.c
	@gcc -o stringClient client.c

test:
	@make all
	@make runServer

PORT=`cat .serverport`

runClient:
	@SERVER_ADDRESS=127.0.0.1 SERVER_PORT=$(PORT) ./stringClient

runServer:
	@./stringServer
