all:
	@g++ -o stringServer server.cc
	@g++ -o stringClient client.cc

test:
	@make all
	@SERVER_ADDRESS=127.0.0.1 SERVER_PORT=8000 ./stringServer
