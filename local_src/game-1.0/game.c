#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>

#define GAMEPAD "/dev/gamepad"


int gamepadDriver;

int main(int argc, char *argv[])
{
	printf("Hello World, I'm game!\n");
	gamepadDriver = open(GAMEPAD, O_RDONLY);
        int numberOfBytesRead __attribute__ ((unused));
        unsigned int buttonValue;
        numberOfBytesRead = read(gamepadDriver, &buttonValue, 2);
	printf("buttonValue fra game: %d  number of bytes read: %d",numberOfBytesRead, buttonValue);
	exit(EXIT_SUCCESS);
}

