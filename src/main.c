/*
 * main.c
 * 
 * Copyright 2018  <pi@raspberrypi>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */


#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <ctype.h>
#include <time.h>
#include <png.h>
#include <inttypes.h>


//cairo
#include <cairo.h>
#include <math.h>
#include "cairolib.h"


#include "kedei_lcd_v50_pi_pigpio.h"


#define MYBUF 4000
// Size of signature
#define SIGNATURE_SIZE 4
// 24bit colors for speed
// bytes in one line
//#define SCREEN_BUF_LINE_LEN LCD_WIDTH * 4
#define STRIDE (LCD_WIDTH * 4)
#define SCREEN_BUFFER_LEN (LCD_HEIGHT * STRIDE)
#define READ_TIMEOUT_US 200

int client_to_server;
char input_buf[MYBUF];
//unsigned char image[STRIDE * LCD_HEIGHT];
uint8_t screen_buffer[SCREEN_BUFFER_LEN];

cairo_t *cr;
cairo_surface_t *surface;

bool compare_signature(uint8_t sig[], char* val)
{
	int len = strlen(val);
	if(len > SIGNATURE_SIZE)
	{
		printf("Error! compare signature with wrong len! Need: %d, found: %s (%d)\n", SIGNATURE_SIZE, val, len);
		return false;
	}
	for(uint8_t i = 0; i < len; i++)
	{
		if(sig[i] != val[i])
			return false;
	}
	return true;
}

// read with timeout
// read_count - real read
bool my_read_count(int fdesc, void *buf, int count, int *read_count)
{
	*read_count = 0;
	struct timespec gettime_now;
	long int start_time;
	long int time_difference;
	clock_gettime(CLOCK_REALTIME, &gettime_now);
	start_time = gettime_now.tv_nsec;
	while(*read_count < count)
	{
		int cur_read = read(fdesc, buf, count - *read_count);
		if(cur_read < 0)
		{
			perror("read");
			return false;
		}
		*read_count += cur_read;
		if(*read_count != count)
		{
			clock_gettime(CLOCK_REALTIME, &gettime_now);
			time_difference = gettime_now.tv_nsec - start_time;
			if (time_difference < 0)
				time_difference += 1000000000;				//(Rolls over every 1 second)
			if (time_difference > (READ_TIMEOUT_US * 1000))		//Delay for # nS
				return false;
		}
	}
	
	return true;
}

bool my_read(int fdesc, void *buf, int count)
{
	int rcnt;
	return my_read_count(fdesc, buf, count, &rcnt);
}

bool skip_read(int skip)
{
	uint8_t buf[128];
	int skipped = 0;
	while(skipped < skip)
	{
		int cur_skip = skip - skipped;
		if(cur_skip > 128)
		{
			cur_skip = 128;
		}
		//printf("\nSkippppp %d\n", cur_skip);
		bool is_readed = my_read(client_to_server, buf, cur_skip);
		if(!is_readed)
		{
			printf("Error in skipping!\n");
			return false;
		}
		skipped += cur_skip;
	}
	return true;
}

void show_part(uint16_t left, uint16_t top, uint16_t width, uint16_t height)
{
	//printf("Swow part left = %u, top = %u, width = %u, height = %u\n\n",
	//	left, top, width, height);
	if(left + width > LCD_WIDTH || top + height > LCD_HEIGHT)
	{
		printf("Error show part! Wrong size!\n");
		return;
	}
	 
	 
	 
	/*printf("First line: \n");
	for(int i = 0; i < SCREEN_BUF_LINE_LEN; i++)
	{
		printf("%02X ", screen_buffer[i]);
	}
	printf("\n");*/
	lcd_setframe(left, top, width, height);
//	uint16_t rowbytes = (width * 3);

	for(uint16_t p = 0; p < height; p++) {
		// p = relative page address (y)
		int cur_line_start = (top + p) * STRIDE;
		for (uint16_t c = 0; c < width; c++) {
			int cur_pos = cur_line_start + (left + c) * 4;
			// c = relative column address (x)
			//fread(buf, 3, 1, f);

			// B buf[0]
			// G buf[1]
			// R buf[2]
			// 18bit color mode
			lcd_colorRGB(screen_buffer[cur_pos + 2], screen_buffer[cur_pos + 1], screen_buffer[cur_pos]);
		}
	}
}

void receive_bmp_file(uint8_t byte2, uint8_t byte3)
{
	/*FILE * fp;
	fp = fopen ("/tmp/receive.bmp", "wb");
	if (! fp) 
	{
		perror("open");
        return;
    }*/
	
	/*
	uint8_t buf[32];
	uint32_t isize = 0, ioffset, iwidth, iheight, ibpp, rowbytes;
	uint16_t show_width, show_height;
	buf[2] = byte2;
	buf[3] = byte3;
	uint8_t rrr = sizeof(buf) - 4;
	bool is_readed = my_read(client_to_server, &buf[4], rrr);
	if(!is_readed)
	{
		printf("Error reading bmp!\n");
		return;
	}
	
	isize =	 	READ_32(buf, 2);
	ioffset = 	READ_32(buf, 0x0A);
	iwidth =	READ_32(buf, 0x12);
	iheight = 	READ_32(buf, 0x16);
	ibpp =		READ_16(buf, 0x1C);
	printf("\n\n");
	printf("File Size: %u\nOffset: %u\nWidth: %u\nHeight: %u\nBPP: %u\n\n",isize,ioffset,iwidth,iheight,ibpp);
	
	uint16_t skip_bytes = ioffset - sizeof(buf);
	//printf("Skip %u bites before start offset\n", skip_bytes);
	if(!skip_read(skip_bytes))		
	{
		printf("Error reading BMP skip!\n");
		return;
	}
	
	show_width = iwidth;
	if(show_width > LCD_WIDTH)
		show_width = LCD_WIDTH;
		
	show_height = iheight;
	if(show_height > LCD_HEIGHT)
		show_height = LCD_HEIGHT;
	
	uint8_t d = (iwidth * 3) % 4;
	rowbytes = (iwidth * 3);
	// needed bytes in line
	int read_line_len = rowbytes;
	if(d > 0)
	{
		rowbytes += 4 - d;
	}
	
	int start_bottom_row = LCD_HEIGHT - 1;
	if(iheight > LCD_HEIGHT)
	{
		// Skip bottom lines
		for(uint16_t p = 0; p < iheight - LCD_HEIGHT; p++)
		{
			if(!skip_read(rowbytes))		
			{
				printf("Error reading BMP!\n");
				return;
			}
		} 
		iheight = LCD_HEIGHT;
	}
	
	if(iheight < LCD_HEIGHT)
	{
		start_bottom_row -= LCD_HEIGHT - iheight;
	}
	
	int pos_in_local_buf;
	// skip not needed bytes in line
	int tail = rowbytes - SCREEN_BUF_LINE_LEN;
	// if rowbytes < SCREEN_BUF_LINE_LEN
	if(tail < 0)
	{
		tail = d;
	}
	else
	{
		read_line_len = SCREEN_BUF_LINE_LEN;
		
	}
	
	//printf("read_line_len = %d, rowbytes = %lu\n", read_line_len, rowbytes);
	//printf("reading line: ");
	for (uint16_t p = 0; p < iheight; p++) 
	{
		pos_in_local_buf = (start_bottom_row - p) * SCREEN_BUF_LINE_LEN;
		//printf("l: %u (pos: %d), ", p, pos_in_local_buf);
				
		
		is_readed = my_read(client_to_server, &screen_buffer[pos_in_local_buf], read_line_len);
		if(!is_readed)
		{
			printf("\nError reading BMP 11!\n");

			return;
		}
		//fwrite(&screen_buffer[pos_in_local_buf], 1, read_line_len, fp);
		
		
		
		
		//screen_buffer[pos_in_local_buf]  		= screen_buffer[pos_in_local_buf + 3] = 0;
		//screen_buffer[pos_in_local_buf + 1] 	= screen_buffer[pos_in_local_buf + 4] = 0;
		//screen_buffer[pos_in_local_buf + 2] 	= screen_buffer[pos_in_local_buf + 5] = 0xff;
		if(tail > 0)
		{
			if(!skip_read(tail))		
			{
				printf("\nError reading BMP!\n");
				return;
			}
		}
		/ * // p = relative page address (y)
		//fpos = ioffset+(p*rowbytes);
		//fseek(f, fpos, SEEK_SET);
		for (c=0;c<iwidth;c++) {
			// c = relative column address (x)
			fread(buf, 3, 1, f);

			// B buf[0]
			// G buf[1]
			// R buf[2]
			// 18bit color mode
			lcd_colorRGB(buf[2], buf[1], buf[0]);
		}* /
	}
	//fclose(fp);
	printf("\n");
	show_part(0, 0, show_width, show_height);*/
}

void export_png()
{
	printf("Export screen to PNG '/tmp/export.png'...\n");
	cairo_surface_write_to_png (surface, "/tmp/export.png");
	printf("Export success\n");
	/*FILE * fp;
	png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    size_t y;
    //png_uint_32 bytes_per_row;
    png_byte **row_pointers = NULL;
    
	printf("Export screen to PNG '/tmp/export.png'...\n");
	
	fp = fopen ("/tmp/export.png", "wb");
	if (! fp) 
	{
		perror("open");
        return;
    }
    
    // Initialize the write struct. 
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        fclose(fp);
        return;
    }

    // Initialize the info struct. 
    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        png_destroy_write_struct(&png_ptr, NULL);
        fclose(fp);
        return;
    }

    // Set up error handling. 
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        return;
    }
    
    // Set image attributes. 
    png_set_IHDR(png_ptr,
                 info_ptr,
                 LCD_WIDTH,
                 LCD_HEIGHT,
                 8,
                 PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);
    // Initialize rows of PNG. 
    
    png_set_bgr(png_ptr);
    
    row_pointers = png_malloc(png_ptr, LCD_HEIGHT * sizeof(png_byte *));
    for (y = 0; y < LCD_HEIGHT; ++y) {
        row_pointers[y] = (png_byte *)(&screen_buffer[y * SCREEN_BUF_LINE_LEN]);
    }
    
    // Actually write the image data. 
    png_init_io(png_ptr, fp);
    png_set_rows(png_ptr, info_ptr, row_pointers);
    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

	png_free(png_ptr, row_pointers);
	
	// Finish writing. 
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);
    printf("Export success\n");*/
}

// if max = 0 - unlimited
uint get_uint(uint max, bool *is_error)
{
	char* endptr;
	*is_error = false;
	errno = 0;
	char *pch = strtok (NULL, " ,.-");
	uint val = strtoumax(pch, &endptr, 10);
	if(errno != 0)
	{
		*is_error = true;
		return 0;
	}
	if(max > 0 && val > max)
	{
		val = max;
	}
	return val;
}

// sample: line w 3 x1 100 y1 300 x2 200 y2 200 r 45 g 55 b 255
// w - strike width
// r, g, b - color, default - black
void draw_line()
{
	uint16_t x1 = 0, x2 = 0, y1 = 0, y2 = 0, w = 1;
	uint8_t r = 0, g = 0, b = 0;
	
	int rcnt;
	my_read_count(client_to_server, input_buf, MYBUF - 1, &rcnt);
	if(rcnt == 0)
		return;
	input_buf[rcnt] = 0;
	bool is_error;
	char *pch = strtok (input_buf, " ,.");
	while (pch != NULL)
	{
		if(strcmp(pch, "w") == 0)
		{
			w = get_uint(0, &is_error);
			if(is_error)
				return;
		}
		else if(strcmp(pch, "x1") == 0)
		{
			x1 = get_uint(LCD_WIDTH - 1, &is_error);
			if(is_error)
				return;
		}
		else if(strcmp(pch, "y1") == 0)
		{
			y1 = get_uint(LCD_HEIGHT - 1, &is_error);
			if(is_error)
				return;
		}
		else if(strcmp(pch, "x2") == 0)
		{
			x2 = get_uint(LCD_WIDTH - 1, &is_error);
			if(is_error)
				return;
		}
		else if(strcmp(pch, "y2") == 0)
		{
			y2 = get_uint(LCD_HEIGHT - 1, &is_error);
			if(is_error)
				return;
		}
		else if(strcmp(pch, "r") == 0)
		{
			r = get_uint(255, &is_error);
			if(is_error)
				return;
		}
		else if(strcmp(pch, "g") == 0)
		{
			g = get_uint(255, &is_error);
			if(is_error)
				return;
		}
		else if(strcmp(pch, "b") == 0)
		{
			b = get_uint(255, &is_error);
			if(is_error)
				return;
		}
		pch = strtok (NULL, " ,.-");
	}
	printf("line w = %u, x1 = %u, y1 = %u, x2 = %u, y2 = %u, r = %u, g = %u, b = %u\n",
		w, x1, y1, x2, y2, r, g, b);
	cairo_line(cr, w, x1, y1, x2, y2, r, g, b);
	uint16_t left, top, width, height;
	if(x1 <= x2)
	{
		left = x1;
		width = x2 - x1;
	}
	else
	{
		left = x2;
		width = x1 - x2;
	}
	if(y1 <= y2)
	{
		top = y1;
		height = y2 - y1;
	}
	else
	{
		top = y2;
		height = y1 - y2;
	}
	printf("show part left = %u, top = %u, width = %u, height = %u\n", left, top, width, height);
	show_part(left, top, width, height);
//	show_part(0, 0, LCD_WIDTH, LCD_HEIGHT);
}

// return: true - exit
bool process_signature(uint8_t sig[])
{
	if(compare_signature(sig, "exit"))
	{
		return true;
		close(client_to_server);
	}
	// export PNG
	if(compare_signature(sig, "expo"))
	{
		export_png();
		return false;
	}
	// Line 
	if(compare_signature(sig, "line"))
	{
		draw_line();
		return false;
	}
	//int read_buf_pos = 0;
	if(compare_signature(sig, "BM"))
	{
		printf("BMP file detected!\n");
		receive_bmp_file(sig[2], sig[3]);
		return false;
	}
	else
	{
		printf("Unknown signature! %s\n", sig);
	}
	uint16_t allRead = 0;
	/*while (1)
	{
	   
		unsigned short readed = read(client_to_server, &buf[read_buf_pos], MYBUF);
		read_buf_pos = 0;
		allRead += readed;
		if(readed == 0)
		{
			printf("Read 0 bytes!. Close!\n");
			break;
		}

		printf("Received: readed = %u last byte = 0x%2X\n", readed, buf[readed - 1]);
		
	  
		/ *if (strcmp("",buf)!=0)
		{
			printf("Received: %s (len = %d)\n", buf, strlen(buf));
			//printf("Sending back...\n");
			//write(server_to_client,buf,BUFSIZ);
		}* /

		// clean buf from any data 
		memset(buf, 0, sizeof(buf));
	}*/
	printf("All Read %u bytes!\n", allRead);
	return false;
}


int fifo_loop()
{
	printf("Server ON..............bufer = %d\n", MYBUF);
	char *client_to_server_name = "/tmp/kedei_lcd_in";

	//int server_to_client;
	char *server_to_client_name = "/tmp/kedei_lcd_out";


	/* create the FIFO (named pipe) */
	printf("Creating FIFO %s ...\n", client_to_server_name);
	if(mkfifo(client_to_server_name, 0666) < 0)
	{
	   printf("Error creating FIFO %s\n", client_to_server_name);
	   //return 1;
	}
	printf("Creating FIFO %s ...\n", server_to_client_name);
	if(mkfifo(server_to_client_name, 0666) < 0)
	{
	   printf("Error creating FIFO %s\n", server_to_client_name);
	   //return 1;
	}

	/* open, read, and display the message from the FIFO */
	
	/*printf("Opening FIFO %s ...\n", myfifo2);
	server_to_client = open(myfifo2, O_WRONLY | O_NONBLOCK);
	if(server_to_client < 0)
	{
	   printf("Error open FIFO %s\n", myfifo2);
	   perror("open");
	   return 1;
	}*/
	uint8_t signature[SIGNATURE_SIZE];
	printf("Server ON.\n");
	while(1)
	{
		printf("Opening FIFO %s ...\n", client_to_server_name);
		client_to_server = open(client_to_server_name, O_RDONLY);//
		if(client_to_server < 0)
		{
			printf("Error open FIFO %s\n", client_to_server_name);
			return 1;
		}
		unsigned short sig_readed = read(client_to_server, signature, SIGNATURE_SIZE);
		if(sig_readed < SIGNATURE_SIZE)
		{
			printf("Error! Signature need at leadt %d bytes!\n", SIGNATURE_SIZE);
			close(client_to_server);
			continue;
		}
		if(process_signature(signature))
		{
			break;
		}
		close(client_to_server);
		
	}

   printf("Server OFF.\n");
   //close(server_to_client);

   unlink(client_to_server_name);
   unlink(server_to_client_name);
   return 0;
}

int main(int argc,char *argv[]) 
{
	//cairo_test();
	//return 0;
	
	
	uint8_t initRotation = 0;
	int aflag = 0;
	int bflag = 0;
	char *cvalue = NULL;
	char *bmpFile = NULL;
	int index;
	int c;

  opterr = 0;

  while ((c = getopt (argc, argv, "ab:c:r:")) != -1)
    switch (c)
      {
      case 'a':
        aflag = 1;
        break;
      case 'b':
        bmpFile = optarg;
        break;
      case 'c':
        cvalue = optarg;
        break;
      case 'r':
		if(strcmp(optarg, "90") == 0)
		{
			initRotation = 1;
		}
		else if(strcmp(optarg, "180") == 0)
		{
			initRotation = 2;
		}
		else if(strcmp(optarg, "270") == 0)
		{
			initRotation = 3;
		}
		else
		{
			fprintf (stderr, "Incorrect rotation '%s'!\n", optarg);
		}
		break;
      case '?':
        if (optopt == 'c')
          fprintf (stderr, "Option -%c requires an argument.\n", optopt);
        else if (isprint (optopt))
          fprintf (stderr, "Unknown option `-%c'.\n", optopt);
        else
          fprintf (stderr,
                   "Unknown option character `\\x%x'.\n",
                   optopt);
        return 1;
      default:
        abort ();
      }

  printf ("rotation = %d, aflag = %d, bflag = %d, cvalue = %s, bmpFile = %s \n",
          initRotation, aflag, bflag, cvalue, bmpFile);

  for (index = optind; index < argc; index++)
    printf ("Non-option argument %s\n", argv[index]);




	printf ("Open LCD\n");
	//delayms(3000);
	if(lcd_open() < 0)
	{
		fprintf (stderr, "Error initialise GPIO!\nNeed stop daemon:\n  sudo killall pigpiod");
		return 1;
	}
	printf ("Init LCD\n");
	//delayms(3000);
	lcd_init(initRotation);

	//printf ("Black LCD\n");
	//delayms(3000);
	//lcd_fill(0); //black out the screen.
	//hex_color_t BG_COLOR =  { 0xd4, 0xd0, 0xc8 };

    surface = cairo_image_surface_create_for_data (screen_buffer, CAIRO_FORMAT_RGB24,
						   LCD_WIDTH, LCD_HEIGHT, STRIDE);
    cr = cairo_create (surface);

    //cairo_rectangle (cr, 0, 0, LCD_WIDTH, LCD_HEIGHT);
    //set_hex_color (cr, BG_COLOR);
    //cairo_fill (cr);

	cairo_test (cr);
	show_part(0, 0, LCD_WIDTH, LCD_HEIGHT);
	fifo_loop();

	/*if(bmpFile != NULL)
	{
		lcd_img(bmpFile, 0, 0);
		
		fifo_loop();
		lcd_close();
		
		return 0;
	}*/
	cairo_destroy (cr);

    cairo_surface_destroy (surface);
    
    lcd_close();
    return 0;
/*
	// 24bit Bitmap only
	lcd_img("kedei_lcd_v50_pi.bmp", 50, 5);
	delayms(500);

	lcd_fillRGB(0xFF, 0x00, 0x00);
	lcd_fillRGB(0x00, 0xFF, 0x00);
	lcd_fillRGB(0xFF, 0xFF, 0x00);
	lcd_fillRGB(0x00, 0x00, 0xFF);
	lcd_fillRGB(0xFF, 0x00, 0xFF);
	lcd_fillRGB(0x00, 0xFF, 0xFF);
	lcd_fillRGB(0xFF, 0xFF, 0xFF);
	lcd_fillRGB(0x00, 0x00, 0x00);

	// 24bit Bitmap only
	lcd_img("kedei_lcd_v50_pi.bmp", 50, 5);
	delayms(500);

	// Demo
	color=0;
	lcd_rotation=0;
	loop();	loop();	loop();	loop();
	loop();	loop();	loop();	loop();
	loop();	loop();	loop();	loop();
	loop();	loop();	loop();	loop();
	loop();

	// 24bit Bitmap only
	lcd_img("kedei_lcd_v50_pi.bmp", 50, 5);

	lcd_close();*/
}


// cairo

