CC=gcc
BFLAGS=-Wall
CFLAGS=-c -Wall

all:
	gcc $(BFLAGS) -o "./dist/lcdout" "./src/main.c" "./src/kedei_lcd_v50_pi.c" "./src/cairolib.c" -I/usr/include/cairo/ -lm -lbcm2835 -lrt -lpng `pkg-config --libs cairo libpng16`

test:
	echo -e 'dtbx\x01\x00\x20\x00\x60\x00\x50\x00\x60\x00\x24\x00\x11\x80\x13\x05\x00Hello' > /tmp/kedei_lcd_in
