CC=gcc -std=gnu11
BFLAGS=-Wall
CFLAGS=-c -Wall `pkg-config --cflags libconfig`
BOUT=-o "./dist/lcdout"
BFILES="./src/main.c" "./src/cairolib.c" "./src/dk_controls.c" "./src/date_time_thread.c"
BINCLUDE=-I/usr/include/cairo/
BLINK=-lrt -lpng -lpthread `pkg-config --libs cairo libpng16 libconfig`

pigpio:
	$(CC) $(BFLAGS) $(BOUT) "./src/kedei_lcd_v50_pi_pigpio.c" $(BFILES) $(BINCLUDE) -lpigpio $(BLINK)
bcm2835:
	$(CC) $(BFLAGS) $(BOUT) "./src/kedei_lcd_v50_pi.c" $(BFILES) $(BINCLUDE) -lm -lbcm2835 $(BLINK)

test:
	sudo python3 test.py ./test.json
