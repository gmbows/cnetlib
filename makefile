all: client.exe server.exe file_client.exe file_server.exe

file_client.exe: bin/cnetlib.o bin/cnet_utility.o bin/file_client.o bin/serializer.o
	C:\Qt\Tools\mingw1120_64\bin\g++.exe bin/cnetlib.o bin/cnet_utility.o bin/file_client.o bin/serializer.o -DASIO_STANDALONE  -lpthread -lminiupnpc -D_REENTRANT -lstdc++fs -std=c++17 -L /lib -I /include -D_WIN32_WINNT=0x0601 -lws2_32 -lwsock32 -lmingw32 -fpermissive -O3 -o file_client.exe

file_server.exe: bin/cnetlib.o bin/cnet_utility.o bin/file_server.o bin/serializer.o
	C:\Qt\Tools\mingw1120_64\bin\g++.exe bin/cnetlib.o bin/cnet_utility.o bin/file_server.o bin/serializer.o -DASIO_STANDALONE  -lpthread -lminiupnpc -D_REENTRANT -lstdc++fs -std=c++17 -L /lib -I /include -D_WIN32_WINNT=0x0601 -lws2_32 -lwsock32 -lmingw32 -fpermissive -O3 -o file_server.exe

client.exe: bin/cnetlib.o bin/cnet_utility.o bin/main_client.o bin/serializer.o
	C:\Qt\Tools\mingw1120_64\bin\g++.exe bin/cnetlib.o bin/cnet_utility.o bin/main_client.o bin/serializer.o -DASIO_STANDALONE  -lpthread -lminiupnpc -D_REENTRANT -lstdc++fs -std=c++17 -L /lib -I /include -D_WIN32_WINNT=0x0601 -lws2_32 -lwsock32 -lmingw32 -fpermissive -O3 -o client.exe

server.exe: bin/cnetlib.o bin/cnet_utility.o bin/main_server.o bin/serializer.o
	C:\Qt\Tools\mingw1120_64\bin\g++.exe bin/cnetlib.o bin/cnet_utility.o bin/main_server.o bin/serializer.o -DASIO_STANDALONE  -lpthread -lminiupnpc -D_REENTRANT -lstdc++fs -std=c++17 -L /lib -I /include -D_WIN32_WINNT=0x0601 -lws2_32 -lwsock32 -lmingw32 -fpermissive -O3 -o server.exe

bin/cnetlib.o: src/cnetlib.cpp src/cnetlib.h  src/cnet_utility.h src/cnet_utility.cpp  
	C:\Qt\Tools\mingw1120_64\bin\g++.exe -c src/cnetlib.cpp -lpthread -lminiupnpc -fpermissive -O3 -lstdc++fs -L /lib -I /include -lmingw32  -D_WIN32_WINNT=0x0601 -lws2_32 -lwsock32 -o bin/cnetlib.o

bin/cnet_utility.o: src/cnet_utility.cpp src/cnet_utility.h  
	C:\Qt\Tools\mingw1120_64\bin\g++.exe -c src/cnet_utility.cpp -lpthread -lminiupnpc -fpermissive -O3 -lstdc++fs -L /lib -I /include -lmingw32  -D_WIN32_WINNT=0x0601 -lws2_32 -lwsock32 -o bin/cnet_utility.o

bin/file_client.o: src/examples/file_client.cpp  
	C:\Qt\Tools\mingw1120_64\bin\g++.exe -c src/examples/file_client.cpp -lpthread -lminiupnpc -fpermissive -O3 -lstdc++fs -L /lib -I /include -lmingw32  -D_WIN32_WINNT=0x0601 -lws2_32 -lwsock32 -o bin/file_client.o

bin/file_server.o: src/examples/file_server.cpp  
	C:\Qt\Tools\mingw1120_64\bin\g++.exe -c src/examples/file_server.cpp -lpthread -lminiupnpc -fpermissive -O3 -lstdc++fs -L /lib -I /include -lmingw32  -D_WIN32_WINNT=0x0601 -lws2_32 -lwsock32 -o bin/file_server.o

bin/main_client.o: src/examples/main_client.cpp  
	C:\Qt\Tools\mingw1120_64\bin\g++.exe -c src/examples/main_client.cpp -lpthread -lminiupnpc -fpermissive -O3 -lstdc++fs -L /lib -I /include -lmingw32  -D_WIN32_WINNT=0x0601 -lws2_32 -lwsock32 -o bin/main_client.o

bin/main_server.o: src/examples/main_server.cpp  
	C:\Qt\Tools\mingw1120_64\bin\g++.exe -c src/examples/main_server.cpp -lpthread -lminiupnpc -fpermissive -O3 -lstdc++fs -L /lib -I /include -lmingw32  -D_WIN32_WINNT=0x0601 -lws2_32 -lwsock32 -o bin/main_server.o

bin/serializer.o: src/serializer.cpp src/serializer.h  src/cnet_utility.h src/cnet_utility.cpp  
	C:\Qt\Tools\mingw1120_64\bin\g++.exe -c src/serializer.cpp -lpthread -lminiupnpc -fpermissive -O3 -lstdc++fs -L /lib -I /include -lmingw32  -D_WIN32_WINNT=0x0601 -lws2_32 -lwsock32 -o bin/serializer.o

