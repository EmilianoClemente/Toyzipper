.PHONY:all test clean
all:
	g++ -o toyzipper zipper.cpp main.cpp -lz
test:
	./toyzipper test.txt test.zip
