All: server client
server:server.o
	g++ -o server server.o
server.o:server.cpp
	g++ -o server.o -c server.cpp


client:client.o
	g++ -o client client.o
client.o:client.cpp
	g++ -o client.o -c client.cpp
