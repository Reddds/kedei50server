CC=gcc
BFLAGS=-Wall
CFLAGS=-c -Wall

all:
	gcc $(BFLAGS) -o "./dist/lcdout" "./src/main.c" "./src/kedei_lcd_v50_pi.c" "./src/cairolib.c" -I/usr/include/cairo/ -lm -lbcm2835 -lrt -lpng `pkg-config --libs cairo libpng16`

test:
	sudo python3 test.py ./test.json
