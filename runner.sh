# compile *.cpp files sfml. ignore warnings
g++ -c game.cpp -w
g++ game.o -o sfml-app -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio -lsfml-network
./sfml-app