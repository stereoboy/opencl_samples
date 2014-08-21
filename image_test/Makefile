

all:  png_test


png_test:png_test.c
	gcc -g -o $@ $^ `pkg-config --libs --cflags libpng gtk+-2.0`

clean: 
	rm -f png_test
