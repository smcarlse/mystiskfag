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

int fbfd;
short *map;
volatile int startGame = 1;
int x = 110;
int life = 0;
int gamepadDriver;
void button_handler(int signal);

struct fb_copyarea rect;
struct sigaction siga = {
	.sa_handler = &button_handler
};

void init()
{
	// Setup which part of the framebuffer that is to be updated
	rect.dx = 0;
	rect.dy = 0;
	rect.width = 320;
	rect.height = 240;

	// Initializes the screen to black 
	int i;
	for (i = 0; i < (320*240); i++) {
		map[i] = 0; 
	}
	
	// Draw three lives in upper right corner
	map[320*2+200] = 3840;
	map[320*2+220] = 3840;
	map[320*2+240] = 3840;

	// Draw the platform to catch stars
	for (i = 320*234+x; i < (320*234)+50+x; i++) {
		map[i] = 65535; 
	}

	// Command driver to update display
	ioctl(fbfd, 0x4680, &rect);	
}

void game_over()
{
	// Stop game
	startGame = 0;

	life = 0;
	
	// Update screen with this area
	rect.dx = 0;
	rect.dy = 0;
	rect.width = 320;
	rect.height = 240;

	// Where to place the platform when starting a new game
	x = 110;

	// Draw the whole screen blue
	int i;
	for (i = 0; i < (320*240); i++) {
		map[i] = 8; 
	}
	
	// Command driver to update display
	ioctl(fbfd, 0x4680, &rect);
}

void falling_star()
{
	// Random number between 0-320. Tells where on the screen the star should fall
	int star = rand()%320;
	int originalStar = star;

	// Update the area where the star is falling
	rect.dx = star;
	rect.dy = 0;
	rect.width = 2;
	rect.height = 235;

	// Update the area where the life is
	struct fb_copyarea lifeRect;
	lifeRect.dx = 195;
	lifeRect.dy = 0;
	lifeRect.width = 50;
	lifeRect.height = 5;
	
	// Set the amount of time to sleep
	struct timespec req = {
		.tv_sec = 0,
		.tv_nsec = 30000
	};

	// The remaining sleep time if the sleep is interrupted
	struct timespec rem;
	
	int sleep;
	while(1)
	{
		// Sleep and if interrupted, sleep the remaining time
		sleep = nanosleep(&req, &rem);
		while (sleep)
		{
			struct timespec new_rem = rem;
			sleep = nanosleep(&new_rem, &rem);
		}
		
		// Draw black above the star, so the star appear to move downwards
		int i;
		for (i = originalStar; i<234*320; i+=320)
		{
			map[i] = 0;
			map[i+1] = 0;
		}
		
		// Increment star with screensize to make the star fall vertically
		star += 320;

		// Check if the star is on the bottom of the screen
		if(star>=235*320)
		{
			// Check if the star hits the platform
			if(320*235+x>=star || star>=320*235+x+50)
			{	
				// Remove a life and update display
				map[320*2+200+life*20] = 0;
				life += 1;
				ioctl(fbfd, 0x4680, &lifeRect);
				
				// Return from function if all lives are lost
				if(life == 3)
				{
					return;
				}
				
			}
			// Generate new random number
			star = rand()%320;
			originalStar = star;
			rect.dx = star;
			
		}else
		{
			// If the star is not at the bottom of the screen, draw it one step further down
			map[star+1] = 65355;
			map[star] = 65355;
			map[star+320] = 65355;
			map[star+321] = 65355;
		}
		// Command driver to update display
		ioctl(fbfd, 0x4680, &rect);	
	}	
}

void button_handler(int signal) {

	// If the startGame variable is set to zero, i.e. the game is over,
	// start the game if a button is pressed
	if (!startGame) 
	{
		startGame = 1;
	}

	int numberOfBytesRead __attribute__ ((unused));
        unsigned int buttonValue;
        numberOfBytesRead = read(gamepadDriver, &buttonValue, 4);

	// Update the area where the platform is
	struct fb_copyarea platformRect;
	platformRect.dx = 0;
	platformRect.dy = 235;
	platformRect.width = 320;
	platformRect.height = 1;

	int i;
	// If the SW3 button is pressed, move platform to the right 
	if(buttonValue == 4 )
	{
		for (i = 320*235; i < (320*236); i++) 
		{
			map[i] = 0; 
		}
		for (i = 320*235+x; i < (320*235)+50+x; i++) 
		{
			map[i] = 65535; 
		}
		if(x < 320-60)
		{
			x += 20;
		}
	// If the SW1 button is pressed, move platfor to the left
	}else if(buttonValue == 1 )
	{
		for (i = 320*235; i < (320*236); i++) 
		{
			map[i] = 0; 
		}
		for (i = 320*235+x; i < (320*235)+50+x; i++) 
		{
			map[i] = 65535; 
		}
		if(x != 0)
		{
			x -= 20;
		}
	}

	// Command driver to update display
	ioctl(fbfd, 0x4680, &platformRect);
	
	return;
}

int main(int argc, char *argv[])
{
	
	fbfd = open("/dev/fb0", O_RDWR);

	size_t screensize = (320 * 240 * 16) / 8;	
	map = (short*) mmap(NULL, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
		
	gamepadDriver = open(GAMEPAD, O_RDONLY);
	if(errno == EACCES)
	{	
		printf("EACCES\n");
	}

	int oflags;
	sigaction(SIGIO, &siga, NULL);
	fcntl(gamepadDriver, F_SETOWN, getpid());
	oflags = fcntl(gamepadDriver, F_GETFL);
	fcntl(gamepadDriver, F_SETFL, oflags | FASYNC);
	
	printf("Starting game \n");
	
	while(1) 
	{
		if (startGame)
		{
			
			init();
			falling_star();
			game_over();
		}

		// Choosen a hight number of seconds to sleep, to save energy
		// The sleep is interrupt if button_handler is called
		sleep(500);
	}
	exit(EXIT_SUCCESS);
}


