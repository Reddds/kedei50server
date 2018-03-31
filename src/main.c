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

#include "kedei_lcd_v50_pi_pigpio.h"

#define MYBUF 4096

int fifo_loop()
{
		printf("Server ON..............bufer = %d\n", MYBUF);
   int client_to_server;
   char *myfifo = "/tmp/client_to_server_fifo";

   //int server_to_client;
   char *myfifo2 = "/tmp/server_to_client_fifo";

   char buf[MYBUF];

   /* create the FIFO (named pipe) */
	printf("Creating FIFO %s ...\n", myfifo);
   if(mkfifo(myfifo, 0666) < 0)
   {
	   printf("Error creating FIFO %s\n", myfifo);
	   //return 1;
   }
	printf("Creating FIFO %s ...\n", myfifo2);
   if(mkfifo(myfifo2, 0666) < 0)
   {
	   printf("Error creating FIFO %s\n", myfifo2);
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

   printf("Server ON.\n");
	while(1)
	{
		printf("Opening FIFO %s ...\n", myfifo);
	   client_to_server = open(myfifo, O_RDONLY);//
	   if(client_to_server < 0)
	   {
		   printf("Error open FIFO %s\n", myfifo);
		   return 1;
	   }
	   unsigned short allRead = 0;
	   while (1)
	   {
		   
		  unsigned short readed = read(client_to_server, buf, MYBUF);
		  allRead += readed;
		  if(readed == 0)
		  {
			  printf("Read 0 bytes!. Close!\n");
			break;
		  }

			printf("Received: readed = %u last byte = 0x%2X\n", readed, buf[readed - 1]);
		  if (strcmp("exit",buf)==0)
		  {
			 printf("Server OFF.\n");
			 break;
		  }
		  
		  else if (strcmp("",buf)!=0)
		  {
			 printf("Received: %s (len = %d)\n", buf, strlen(buf));
			 //printf("Sending back...\n");
			 //write(server_to_client,buf,BUFSIZ);
		  }

		  /* clean buf from any data */
		  memset(buf, 0, sizeof(buf));
	   }
	   printf("All Read %u bytes!\n", allRead);
	   close(client_to_server);
	}

   
   //close(server_to_client);

   unlink(myfifo);
   unlink(myfifo2);
   return 0;
}

int main(int argc,char *argv[]) 
{


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

	printf ("Black LCD\n");
	//delayms(3000);
	lcd_fill(0); //black out the screen.

	if(bmpFile != NULL)
	{
		lcd_img(bmpFile, 0, 0);
		
		fifo_loop();
		lcd_close();
		
		return 0;
	}

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


