all:
	@gcc -o stringServer server.c
	@gcc -o stringClient client.c
	@./stringServer

ADDR=`cat .serveraddr`
PORT=`cat .serverport`

runClient:
	@SERVER_ADDRESS=$(ADDR) SERVER_PORT=$(PORT) ./stringClient
