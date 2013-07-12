all: librpc.a binder 

librpc.a: rpc.o initialization.o message.o
	ar rcs librpc.a rpc.o initialization.o message.o

binder: binder.o 
	g++ -g -L. binder.o -lrpc -o binder

rpc.o: rpc.cpp error.h binder.h initialization.h message.h rpc.h server_func.h cache.h
	g++ -pthread -c rpc.cpp

binder.o: binder.cpp binder.h initialization.h message.h
	g++ -g -c binder.cpp

initialization.o: initialization.cpp binder.h initialization.h error.h
	g++ -g -c initialization.cpp

message.o: message.cpp message.h rpc.h
	g++ -g -c message.cpp

clean:
	rm -f *.o binder librpc.a	
