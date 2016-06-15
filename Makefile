all:
	@g++ -o stringServer server.cpp
	@g++ -o stringClient -std=c++11 -pthread client.cpp
	@./stringServer

ADDR=`cat .serveraddr`
PORT=`cat .serverport`

runClient:
	@SERVER_ADDRESS=$(ADDR) SERVER_PORT=$(PORT) ./stringClient

runMacClient:
	@SERVER_ADDRESS=127.0.0.1 SERVER_PORT=$(PORT) ./stringClient
