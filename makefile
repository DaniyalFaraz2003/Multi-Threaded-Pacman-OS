run:
	clear
	g++ -c main.cpp
	g++ main.o -o sfml-app -lsfml-graphics -lsfml-window -lsfml-system -lpthread
	./sfml-app