CC=g++
default: ShadowCasting2D.cpp
	$(CC) -o raycaster.out ShadowCasting2D.cpp -lX11 -lGL -lpthread -lpng -lstdc++fs -std=c++17

release: ShadowCasting2D.cpp
	$(CC) -o raycaster.out ShadowCasting2D.cpp -lX11 -lGL -lpthread -lpng -lstdc++fs -std=c++17 -O3
