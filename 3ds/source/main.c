#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <3ds.h>
#include <3ds/services/ir.h>
#include "irc_wrapper.h"

#define BITRATE 3 // 115200 baud


#define ARR_SIZE 8
int main(int argc, char* argv[])
{
	gfxInitDefault();
	consoleInit(GFX_TOP, NULL);

	printf("pokewalker\n");
	if (IRCInit(BITRATE) != 0) {
		printf("IRCInit failed\n");
	}

	// Main loop
	u8 arr[ARR_SIZE];
	for (int i = 0; i < ARR_SIZE; i += 1) arr[i] = 0;
	while (aptMainLoop())
	{
		gspWaitForVBlank();
		gfxSwapBuffers();
		hidScanInput();

		// u8 arr[] = { 1, 2, 3, 4 };
		// IRCSend(arr, 4);
		
		u32 receivedBytes;
		IRCReceive(arr, ARR_SIZE, &receivedBytes, 900);
		printf("received bytes: %d\n", receivedBytes);

		for (int i = 0; i < ARR_SIZE; i += 1) {
			printf("%x  ", arr[i] ^ 0xAA);
		}
		printf("\n\n");

		// Your code goes here
		u32 kDown = hidKeysDown();
		if (kDown & KEY_B)
			break; // break in order to return to hbmenu
	}

	IRCShutdown();

	gfxExit();
	return 0;
}
