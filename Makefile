CC=gcc
BFLAGS=-Wall
CFLAGS=-c -Wall
BOUT=-o "./dist/lcdout"
BFILES="./src/main.c" "./src/cairolib.c"
BINCLUDE=-I/usr/include/cairo/
BLINK=-lrt -lpng -lpthread `pkg-config --libs cairo libpng16`

pigpio:
	$(CC) $(BFLAGS) $(BOUT) "./src/kedei_lcd_v50_pi_pigpio.c" $(BFILES) $(BINCLUDE) -lpigpio $(BLINK)
bcm2835:
	$(CC) $(BFLAGS) $(BOUT) "./src/kedei_lcd_v50_pi.c" $(BFILES) $(BINCLUDE) -lm -lbcm2835 $(BLINK)

test:
	sudo python3 test.py ./test.json
