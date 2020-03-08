CC = g++
FLAGS = -Wall -Wextra -g -std=c++11 -Werror
LIBSOCKET = -lnsl
CLIENT = subscriber
SERVER = server
STORAGE = storage

.PHONY: clean

build: $(SERVER) $(CLIENT)

$(SERVER): $(SERVER).o $(STORAGE).o
	$(CC) $(FLAGS) $(LIBSOCKET) $^ -o $@

$(SERVER).o: $(SERVER).cpp
	$(CC) $(FLAGS) $< -c

$(CLIENT): $(CLIENT).o
	$(CC) $(FLAGS) $(LIBSOCKET) $< -o $@

$(CLIENT).o: $(CLIENT).cpp
	$(CC) $(FLAGS) $< -c

$(STORAGE).o: $(STORAGE).cpp $(STORAGE).h
	$(CC) $(FLAGS) $< -c

clean:
	rm -rf *.o *~ $(SERVER) $(CLIENT)
