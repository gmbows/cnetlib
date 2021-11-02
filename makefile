main.exe: bin/CNetLib.o bin/Common.o bin/Main.o bin/Utility.o
	g++  bin/CNetLib.o bin/Common.o bin/Main.o bin/Utility.o -std=c++17 -DASIO_STANDALONE  -lpthread -I/usr/include/SDL2 -D_REENTRANT -lpthread -I /include/SDL2 -L /lib -D_WIN32_WINNT=0x0601 -lws2_32 -lwsock32 -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -fpermissive -O3 -o main.exe

bin/CNetLib.o: src/CNetLib.cpp src/CNetLib.h  src/Utility.h src/Utility.cpp  
	g++  -std=c++17 -c src/CNetLib.cpp -lpthread -fpermissive -O3 -I /include/SDL2 -L /lib -lmingw32  -D_WIN32_WINNT=0x0601 -lws2_32 -lwsock32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -o bin/CNetLib.o

bin/Common.o: src/Common.cpp src/Common.h  
	g++  -std=c++17 -c src/Common.cpp -lpthread -fpermissive -O3 -I /include/SDL2 -L /lib -lmingw32  -D_WIN32_WINNT=0x0601 -lws2_32 -lwsock32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -o bin/Common.o

bin/Main.o: src/Main.cpp  
	g++  -std=c++17 -c src/Main.cpp -lpthread -fpermissive -O3 -I /include/SDL2 -L /lib -lmingw32  -D_WIN32_WINNT=0x0601 -lws2_32 -lwsock32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -o bin/Main.o

bin/Utility.o: src/Utility.cpp src/Utility.h  
	g++  -std=c++17 -c src/Utility.cpp -lpthread -fpermissive -O3 -I /include/SDL2 -L /lib -lmingw32  -D_WIN32_WINNT=0x0601 -lws2_32 -lwsock32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -o bin/Utility.o

