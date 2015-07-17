cc = g++ -g -O0 -std=c++11 -pthread -Wl,--no-as-needed -g -D_REENTRANT
server_objects = server.o convert.o hash_cache.o

server: $(server_objects)
	$(cc) $(server_objects) -o server

server.o: server.cpp convert.h hash_cache.h
	$(cc) -c server.cpp -o server.o

client: client.o convert.o
	$(cc) client.o convert.o -o client
client.o: client.cpp node.h convert.h
	$(cc) -c client.cpp -o client.o

convert.o: convert.cpp node.h convert.h
	$(cc) -c convert.cpp -o convert.o
hash_cache.o: hash_cache.cpp hash_cache.h
	$(cc) -c hash_cache.cpp -o hash_cache.o

clean:
	rm *.o
clean_data:
	rm *.dat
