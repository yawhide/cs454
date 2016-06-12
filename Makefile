all:
	@gcc -o stringServer server.c
	@gcc -o stringClient client.c

test:
	@make all
	@make runServer

ADDR=`cat .serveraddr`
PORT=`cat .serverport`

runClient:
	@SERVER_ADDRESS=$(ADDR) SERVER_PORT=$(PORT) ./stringClient

runServer:
	@./stringServer
