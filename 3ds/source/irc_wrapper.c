#include <3ds.h>
#include <3ds/services/ir.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include "irc_wrapper.h"

#define SHAREDMEM_SIZE 0x1000
#define SHAREDMEM_ALIGNMENT 0x1000
static void* sharedMem;
static u8 transferBuffer[0x208];
static Handle recvEvent;

int IRCInit(u8 bitrate)
{
	sharedMem = memalign(SHAREDMEM_ALIGNMENT, SHAREDMEM_SIZE);
  if (!sharedMem) {
    printf("IRCInit: memalign failed\n");
    return -1;
  }

	if (R_FAILED(iruInit(sharedMem, SHAREDMEM_SIZE))) {
		printf("IRCInit: IRU_Initialize failed\n");
    return -1;
	}

	if (R_FAILED(IRU_SetBitRate(bitrate))) {
    printf("IRCInit: SetBitRate failed\n");
    return -1;
  }

	if (R_FAILED(IRU_GetRecvFinishedEvent(&recvEvent))) {
    printf("IRCInit: GetRecvFinishedEvent failed\n");
    return -1;
  }

  return 0;
}

int IRCShutdown(void)
{
  iruExit();
  free(sharedMem);
  return 0;
}

int IRCSend(u8* data, u32 size)
{
  memcpy(transferBuffer, data, size);

  // Send the packet
  return R_FAILED(iruSendData(transferBuffer, size, true)) ? -1 : 0;
}

int IRCReceive(void* data, u32 bufferSize, u32* receivedSize, u32 timeout)
{
  if (R_FAILED(IRU_StartRecvTransfer(bufferSize, 0))) {
    return -1;
  }

  svcWaitSynchronization(recvEvent, timeout * 1000000llu);
  svcClearEvent(recvEvent);

  u32 transferCount;
  if (R_FAILED(IRU_WaitRecvTransfer(&transferCount))) {
    return -1;
  }
  if (!transferCount) {
    return 0;
  }

  u8* buffer = (u8*) sharedMem;
  
  *receivedSize = transferCount;
  memcpy(data, buffer, transferCount);
  
  return 0;
}