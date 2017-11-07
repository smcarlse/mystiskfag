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

void button_handler(int signal) {
	printf("dette er et signal \n");
	return;
}

struct sigaction siga = {
	.sa_handler = &button_handler
};

int main(int argc, char *argv[])
{
	fbfd = open("/dev/fb0", O_RDWR);
	
	sigaction(SIGIO, &siga, NULL);
	fcntl(fbfd, F_SETOWN, getpid());
	oflags = fcntl(fbfd, F_GETFL);
	fcntl(fbfd, F_SETFL, oflags | FASYNC);	
	
	
	map = (short*) mmap(NULL, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
	printf("Mapping %p \n", map);

	// setup which part of the framebuffer that is to be refreshed 
	// for performance reasons, use as small rectangle as possible 
	struct fb_copyarea rect;
	rect.dx = 0;
	rect.dy = 0;
	rect.width = 320;
	rect.height = 240;
	
	int i;
	int j;
	for (i = 320*235; i < (320*235)+50; i++) {
		map[i] = 8; 
	}	

	// command driver to update display
	ioctl(fbfd, 0x4680, &rect);

	printf("Hello World, I'm game!\n");
	gamepadDriver = open(GAMEPAD, O_RDONLY);
	if(errno == EACCES) printf("EACCES\n");
	printf("error 1: %s \n", strerror(errno));
	printf("gamepaddriver: %d \n", gamepadDriver);
        int numberOfBytesRead __attribute__ ((unused));
        unsigned int buttonValue;
        numberOfBytesRead = read(gamepadDriver, &buttonValue, 4);
	printf("buttonValue fra game: %d  number of bytes read: %d \n",numberOfBytesRead, buttonValue);
	printf("error 2: %d \n", errno);
	while(1);
	exit(EXIT_SUCCESS);
}

