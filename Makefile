OUT=chat_client chat_coordinator chat_server
DEPENDENCIES=Reader.cpp

# compiler
CC=g++

#  Main target
all: coordinator session client

coordinator:
	$(CC) $(FLAGS) -o chat_coordinator chat_coordinator.cpp $(DEPENDENCIES)

session:
	$(CC) $(FLAGS) -o chat_server chat_server.cpp $(DEPENDENCIES)

client:
	$(CC) $(FLAGS) -o chat_client chat_client.cpp $(DEPENDENCIES)

clean:
	rm $(OUT)
