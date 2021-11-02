main: bin/Utility.o bin/CNetLib.o bin/Common.o bin/Main.o
	g++  bin/Utility.o bin/CNetLib.o bin/Common.o bin/Main.o -std=c++17 -DASIO_STANDALONE  -lpthread -I/usr/include/SDL2 -D_REENTRANT -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf  -fpermissive -O3 -o main

bin/Utility.o: src/Utility.cpp src/Utility.h 
	g++  -c src/Utility.cpp -std=c++17 -DASIO_STANDALONE  -lpthread -I/usr/include/SDL2 -D_REENTRANT -fpermissive -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -o bin/Utility.o

bin/CNetLib.o: src/CNetLib.cpp src/CNetLib.h 
	g++  -c src/CNetLib.cpp -std=c++17 -DASIO_STANDALONE  -lpthread -I/usr/include/SDL2 -D_REENTRANT -fpermissive -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -o bin/CNetLib.o

bin/Common.o: src/Common.cpp src/Common.h 
	g++  -c src/Common.cpp -std=c++17 -DASIO_STANDALONE  -lpthread -I/usr/include/SDL2 -D_REENTRANT -fpermissive -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -o bin/Common.o

bin/Main.o: src/Main.cpp  
	g++  -c src/Main.cpp -std=c++17 -DASIO_STANDALONE  -lpthread -I/usr/include/SDL2 -D_REENTRANT -fpermissive -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -o bin/Main.o

