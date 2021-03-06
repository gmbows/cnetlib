all: client.exe server.exe

client.exe: bin/cnetlib.o bin/cnet_utility.o bin/main_client.o bin/serializer.o
	C:\Qt\Tools\mingw1120_64\bin\g++.exe bin/cnetlib.o bin/cnet_utility.o bin/main_client.o bin/serializer.o -DASIO_STANDALONE  -lpthread -D_REENTRANT -lstdc++fs -std=c++17 -L /lib -I /include -D_WIN32_WINNT=0x0601 -lws2_32 -lwsock32 -lmingw32 -fpermissive -O3 -o client.exe

server.exe: bin/cnetlib.o bin/cnet_utility.o bin/main_server.o bin/serializer.o
	C:\Qt\Tools\mingw1120_64\bin\g++.exe bin/cnetlib.o bin/cnet_utility.o bin/main_server.o bin/serializer.o -DASIO_STANDALONE  -lpthread -D_REENTRANT -lstdc++fs -std=c++17 -L /lib -I /include -D_WIN32_WINNT=0x0601 -lws2_32 -lwsock32 -lmingw32 -fpermissive -O3 -o server.exe

bin/cnetlib.o: src/cnetlib.cpp src/cnetlib.h  src/cnet_utility.h src/cnet_utility.cpp  
	C:\Qt\Tools\mingw1120_64\bin\g++.exe -c src/cnetlib.cpp -lpthread -fpermissive -O3 -L /lib -I /include -lmingw32 -lstdc++fs -D_WIN32_WINNT=0x0601 -lws2_32 -lwsock32 -o bin/cnetlib.o

bin/cnet_utility.o: src/cnet_utility.cpp src/cnet_utility.h  
	C:\Qt\Tools\mingw1120_64\bin\g++.exe -c src/cnet_utility.cpp -lpthread -fpermissive -O3 -L /lib -I /include -lmingw32 -lstdc++fs -D_WIN32_WINNT=0x0601 -lws2_32 -lwsock32 -o bin/cnet_utility.o

bin/main_client.o: src/main_client.cpp  
	C:\Qt\Tools\mingw1120_64\bin\g++.exe -c src/main_client.cpp -lpthread -fpermissive -O3 -L /lib -I /include -lmingw32 -lstdc++fs -D_WIN32_WINNT=0x0601 -lws2_32 -lwsock32 -o bin/main_client.o

bin/main_server.o: src/main_server.cpp  
	C:\Qt\Tools\mingw1120_64\bin\g++.exe -c src/main_server.cpp -lpthread -fpermissive -O3 -L /lib -I /include -lmingw32 -lstdc++fs -D_WIN32_WINNT=0x0601 -lws2_32 -lwsock32 -o bin/main_server.o

bin/serializer.o: src/serializer.cpp src/serializer.h  src/cnet_utility.h src/cnet_utility.cpp  
	C:\Qt\Tools\mingw1120_64\bin\g++.exe -c src/serializer.cpp -lpthread -fpermissive -O3 -L /lib -I /include -lmingw32 -lstdc++fs -D_WIN32_WINNT=0x0601 -lws2_32 -lwsock32 -o bin/serializer.o

clean:
	del bin\*