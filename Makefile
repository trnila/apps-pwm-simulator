all:
	g++ -g main.cpp -lpthread -fsanitize=address -o main

clean:
	rm main


