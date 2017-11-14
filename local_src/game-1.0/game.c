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
#include <time.h>

#define GAMEPAD "/dev/gamepad"


int gamepadDriver;
int fbfd;
size_t screensize = (320 * 240 * 16) / 8;
short *map;
int oflags;
int e;
int x = 110;
int star = 0; 
int life = 0;
struct fb_copyarea rect;
volatile int startGame = 1;




void init()
{
	rect.dx = 0;
	rect.dy = 0;
	rect.width = 320;
	rect.height = 240;


	int i;

	for (i = 0; i < (320*240); i++) {
		map[i] = 0; 
	}
	
	map[320*2+200] = 3840;
	map[320*2+220] = 3840;
	map[320*2+240] = 3840;

	for (i = 320*234+x; i < (320*234)+50+x; i++) {
		map[i] = 65535; 
	}

	// command driver to update display
	ioctl(fbfd, 0x4680, &rect);	
}

void game_over()
{
	startGame = 0;
	life = 0;
	printf("gameOver:((((");
	rect.dx = 0;
	rect.dy = 0;
	rect.width = 320;
	rect.height = 240;
	x = 110;

	int i;

	for (i = 0; i < (320*240); i++) {
		map[i] = 8; 
	}
	
	ioctl(fbfd, 0x4680, &rect);
	
}

void falling_star()
{
	star = rand()%320;
	int originalStar = star;
	printf("hei dette er et random tall: %d \n", star);
	rect.dx = star;
	rect.dy = 0;
	rect.width = 2;
	rect.height = 235;
	int count = 0;

	struct fb_copyarea lyfeRect;
	lyfeRect.dx = 195;
	lyfeRect.dy = 0;
	lyfeRect.width = 50;
	lyfeRect.height = 5;
	
 	int sleep;
	struct timespec req = {
	.tv_sec = 0,
	.tv_nsec = 30000
	};

	struct timespec rem;
	
	while(1)
	{
		sleep = nanosleep(&req, &rem);
		while (sleep != 0){
			struct timespec rem2 = rem;
			sleep = nanosleep(&rem2, &rem);
		}
		//count += 1;
		//if (count %30000 == 0){
			int k;
			for (k = originalStar; k<234*320; k+=320){
				map[k]=0;
				map[k+1]=0;
			}
			star += 320;

			if(star>=235*320){
				printf("x: %d, star: %d \n", x+320*235, star);
				if(320*235+x<=star && star<=320*235+x+50)
				{
					printf("poeng mann \n");
				}else{
					
					map[320*2+200+life*20] = 0;
					life+=1;
					printf("lyfe  \n");
					ioctl(fbfd, 0x4680, &lyfeRect);
					if(life == 3){
						return;
					}
					
				}
				star = rand()%320;
				originalStar = star;
				rect.dx = star;
				
			}else{
				map[star +1] = 65355;
				map[star] = 65355;
				map[star+320] = 65355;
				map[star+321] = 65355;
			}
			ioctl(fbfd, 0x4680, &rect);
			
		//}
			
	}	
}



void button_handler(int signal) {
	startGame = 1;

	int numberOfBytesRead __attribute__ ((unused));
        unsigned int buttonValue;
        numberOfBytesRead = read(gamepadDriver, &buttonValue, 4);
	// setup which part of the framebuffer that is to be refreshed 
	// for performance reasons, use as small rectangle as possible 

	struct fb_copyarea buttonRect;
	buttonRect.dx = 0;
	buttonRect.dy = 235;
	buttonRect.width = 320;
	buttonRect.height = 1;


	int i;

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
	ioctl(fbfd, 0x4680, &buttonRect);
	
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
	
	while(1) 
	{
		
		if (startGame)
		{
			
			init();
		
			falling_star();

			game_over();
		}
		sleep(500);
	
	}
	exit(EXIT_SUCCESS);
}


