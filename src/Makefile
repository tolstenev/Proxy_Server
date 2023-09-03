CC		:= g++
CFLAGS	:= -std=c++11
SERVER	:= server
CLIENT	:= client

all: $(SERVER) $(CLIENT)

$(SERVER): $(SERVER).cpp
	$(CC) $(CFLAGS) -o $(SERVER) $(SERVER).cpp -lpq

$(CLIENT): $(CLIENT).cpp
	$(CC) $(CFLAGS) -o $(CLIENT) $(CLIENT).cpp -lpq

clean:
	rm -f $(SERVER) $(CLIENT)
