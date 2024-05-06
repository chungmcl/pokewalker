#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <3ds.h>
#include <3ds/services/ir.h>
#include "irc_wrapper.h"
#include "commands.h"

#define BITRATE 3 // 115200 baud
u8 session_id[4] = { 0x0, 0x0, 0x0, 0x0 };
Handle iruHandle;

uint16_t calculate_and_insert_checksum(uint8_t* bytes, uint32_t len) {
  uint32_t sum = 0;
  for (int i = 0; i < len; i++) {
    // bytes 2 and 3 are actually the checksums themselves -- don't
    // include in calculation.
		uint16_t byte = (i == 2 || i == 3) ? 0x00 : bytes[i];
		
		if (!(i & 1))
			byte <<= 8;
		sum += byte;
	}
  while (sum >> 16) {
    sum = (uint16_t)sum + (sum >> 16);
  }
  sum += 2;
  bytes[2] = (uint8_t)sum;
  bytes[3] = (sum >> 8);
  return (uint16_t)sum;
}

void insert_session_id(uint8_t* bytes) {
  for (int i = 0; i < 4; i += 1) {
    bytes[i+4] = session_id[i] ^ 0xAA;
  }
}

Result send_bytes(u8* bytes, u32 len) {
  insert_session_id(bytes);
  calculate_and_insert_checksum(bytes, len);
	// pauseMs(2);
  for (u32 i = 0; i < len; i += 1) 
    bytes[i] = bytes[i] ^ 0xAA;
	return iruSendData(bytes, len, false);
	// iruSendData(bytes, len, false); svcClearEvent(iruHandle); return 0;
	// IRCSend(bytes, len);
}

/**
 * @brief LINE 131 ir.c
 * 
 */

#define BUF_SIZE 64
int main(int argc, char* argv[])
{
	gfxInitDefault();
	consoleInit(GFX_TOP, NULL);

	printf("pokewalker\n");

	u8* recvBuf = IRCInit(BITRATE); // read only?
	iruHandle = iruGetServHandle();
	// if (recvBuf == NULL) {
	// 	printf("IRCInit failed\n");
	// }
	// IRCInit(BITRATE); u8 recvBuf[32]; for (int i = 0; i < 32; i += 1) recvBuf[i] = 0;
	
	u32 receivedBytes;
	// u8 _buf[BUF_SIZE]; for (int i = 0; i < BUF_SIZE; i += 1) _buf[i] = 0;
	u8* buf = recvBuf;
	while (aptMainLoop())
	{
		gspWaitForVBlank();
		gfxSwapBuffers();
		hidScanInput();

		// u8 arr[] = { 1, 2, 3, 4 };
		// IRCSend(arr, 4);

		
		// send_bytes(handshake, sizeof(handshake));
		// IRCReceive(&receivedBytes, 1, 5000, recvBuf);
		IRCReceive(&receivedBytes, 1, 3000, buf);
		// send_bytes(handshake, sizeof(handshake));
		iruSendData(handshake, sizeof(handshake), false);
		// svcSleepThread(2 * 1000000llu);
		// IRCReceive(&receivedBytes, 8, 3000, buf);
		Result res = IRU_StartRecvTransfer(8, 0b00000000);
		if (R_FAILED(res)) {
    	printf("failed 2 err %lX\n", (s32)res);
			svcSleepThread(10000 * 1000000llu);
    	break;
  	}
  	svcWaitSynchronization(iruHandle, 3000 * 1000000llu);
  	svcClearEvent(iruHandle);
  	if (R_FAILED(IRU_WaitRecvTransfer(&receivedBytes))) {
  	  printf("neggy one 2\n");
			svcSleepThread(1000 * 1000000llu);
  	  break;
  	}

		// IRCReceive(&receivedBytes, 8, 3000, buf + receivedBytes);
		// iruRecvData(buf, 5, 0, &receivedBytes, true);
		// printf("recv: %ld\n", receivedBytes);

		// send_bytes(handshake, sizeof(handshake));
		// 
		// IRCReceive(&receivedBytes, 8, 2000, recvBuf);
		// // printf("recv: %ld\n", receivedBytes);

		for (int i = 0; i < BUF_SIZE; i += 1) {
			printf("%x  ", buf[i]);
		}
		printf("\n\n");

		// Your code goes here
		u32 kDown = hidKeysDown();
		if (kDown & KEY_B)
			break; // break in order to return to hbmenu
	}

	svcClearEvent(iruHandle);
	IRCShutdown();
	gfxExit();
	return 0;
}
