OUT=chat_client chat_coordinator chat_server
DEPENDENCIES=Reader.cpp

# compiler
CC=g++

UNAME := $(shell uname)

# if using OSX use this flag so the call to select doesn't break
ifeq ($(UNAME), Darwin)
	# FLAGS=-D_DARWIN_UNLIMITED_SELECT=1
endif

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
