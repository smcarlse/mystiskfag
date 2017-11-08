#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <signal.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define GAMEPAD "/dev/gamepad"


int gamepadDriver;
int fbfd;
size_t screensize = (320 * 240 * 16) / 8;
short *map;
int oflags;
int e;
int x = 0;
int star = 0; 

void button_handler(int signal) {
	printf("dette er et signal \n");

	int numberOfBytesRead __attribute__ ((unused));
        unsigned int buttonValue;
        numberOfBytesRead = read(gamepadDriver, &buttonValue, 4);
	printf("buttonValue fra game: %d  number of bytes read: %d \n",numberOfBytesRead, buttonValue);
		// setup which part of the framebuffer that is to be refreshed 
	// for performance reasons, use as small rectangle as possible 
	struct fb_copyarea rect;
	rect.dx = 0;
	rect.dy = 235;
	rect.width = 320;
	rect.height = 1;


	int i;
	int j;

	if(buttonValue == 4 ){
		for (i = 320*235; i < (320*236); i++) {
			map[i] = 0; 
		}
		for (i = 320*235+x; i < (320*235)+50+x; i++) {
			map[i] = 65535; 
		}
		if(x < 320-60){
			x += 20;
		}
	}else if(buttonValue == 1 ){
		for (i = 320*235; i < (320*236); i++) {
			map[i] = 0; 
		}
		for (i = 320*235+x; i < (320*235)+50+x; i++) {
			map[i] = 65535; 
		}
		if(x != 0){
			x -= 20;
		}
	}
	// command driver to update display
	ioctl(fbfd, 0x4680, &rect);
	
	return;
}

struct sigaction siga = {
	.sa_handler = &button_handler
};

int main(int argc, char *argv[])
{
	fbfd = open("/dev/fb0", O_RDWR);
	
	map = (short*) mmap(NULL, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
	printf("Mapping %p \n", map);
		

	printf("Hello World, I'm game!\n");
	gamepadDriver = open(GAMEPAD, O_RDONLY);
	if(errno == EACCES) printf("EACCES\n");
	printf("error 1: %s \n", strerror(errno));
	printf("gamepaddriver: %d \n", gamepadDriver);

	
	printf("error 2: %d \n", errno);

		
	sigaction(SIGIO, &siga, NULL);
	e = fcntl(gamepadDriver, F_SETOWN, getpid());
	printf("åpner signal eller noe %d \n", e);
	oflags = fcntl(gamepadDriver, F_GETFL);
	e = fcntl(gamepadDriver, F_SETFL, oflags | FASYNC);
	printf("fcntl %d \n", e);
	printf("error 1: %s \n", strerror(errno));
	
	struct fb_copyarea rect;
	rect.dx = 0;
	rect.dy = 0;
	rect.width = 320;
	rect.height = 240;

	int i;

	for (i = 0; i < (320*240); i++) {
		map[i] = 0; 
	}
	// command driver to update display
	ioctl(fbfd, 0x4680, &rect);
	
	star = rand()%320;
	printf("hei dette er et random tall: %d \n", star);
	rect.dx = star;
	rect.dy = 0;
	rect.width = 1;
	rect.height = 240;
	int count = 0;
	
	while(1)
	{
		count += 1;
		if (count %30000 == 0){
		int k;
		for (k = star; k<240*320; k+=320){
			map[k]=0;
		}
		map[star] = 65355;
		star += 320;

		ioctl(fbfd, 0x4680, &rect);
		}
			
	}
	exit(EXIT_SUCCESS);
}

