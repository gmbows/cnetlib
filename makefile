main.exe: bin/cnetlib.o bin/cnet_utility.o bin/main.o bin/serializer.o
	g++ bin/cnetlib.o bin/cnet_utility.o bin/main.o bin/serializer.o -DASIO_STANDALONE  -lpthread -D_REENTRANT -lstdc++fs -std=c++17 -L /lib -I /include -D_WIN32_WINNT=0x0601 -lws2_32 -lwsock32 -lmingw32 -fpermissive -O3 -o main.exe

bin/cnetlib.o: src/cnetlib.cpp src/cnetlib.h  
	g++ -c src/cnetlib.cpp -lpthread -fpermissive -O3 -L /lib -I /include -lmingw32  -D_WIN32_WINNT=0x0601 -lws2_32 -lwsock32 -o bin/cnetlib.o

bin/cnet_utility.o: src/cnet_utility.cpp src/cnet_utility.h  
	g++ -c src/cnet_utility.cpp -lpthread -fpermissive -O3 -L /lib -I /include -lmingw32  -D_WIN32_WINNT=0x0601 -lws2_32 -lwsock32 -o bin/cnet_utility.o

bin/main.o: src/main.cpp  
	g++ -c src/main.cpp -lpthread -fpermissive -O3 -L /lib -I /include -lmingw32  -D_WIN32_WINNT=0x0601 -lws2_32 -lwsock32 -o bin/main.o

bin/serializer.o: src/serializer.cpp src/serializer.h  src/cnet_utility.h src/cnet_utility.cpp  
	g++ -c src/serializer.cpp -lpthread -fpermissive -O3 -L /lib -I /include -lmingw32  -D_WIN32_WINNT=0x0601 -lws2_32 -lwsock32 -o bin/serializer.o

