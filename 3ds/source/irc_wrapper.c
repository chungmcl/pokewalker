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

u8* IRCInit(u8 bitrate)
{
	sharedMem = memalign(SHAREDMEM_ALIGNMENT, SHAREDMEM_SIZE);
  if (!sharedMem) {
    printf("IRCInit: memalign failed\n");
    return NULL;
  }

	if (R_FAILED(iruInit(sharedMem, SHAREDMEM_SIZE))) {
		printf("IRCInit: IRU_Initialize failed\n");
    return NULL;
	}

	if (R_FAILED(IRU_SetBitRate(bitrate))) {
    printf("IRCInit: SetBitRate failed\n");
    return NULL;
  }

	if (R_FAILED(IRU_GetRecvFinishedEvent(&recvEvent))) {
    printf("IRCInit: GetRecvFinishedEvent failed\n");
    return NULL;
  }

  return (u8*)sharedMem;
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

int IRCReceive(u32* actualReadSize, u32 readSize, u32 timeout, u8* out)
{
  Result res = IRU_StartRecvTransfer(readSize, 0b00000000);
  if (R_FAILED(res)) {
    printf("error %ld: failed to read %ld bytes\n", res, readSize);
    return -1;
  }
  svcWaitSynchronization(recvEvent, timeout * 1000000llu);
  svcClearEvent(recvEvent);
  
  if (R_FAILED(IRU_WaitRecvTransfer(actualReadSize))) {
    printf("neggy one\n");
    return -1;
  }
  
  return 0;
}

// int IRCReceive(void* data, u32 bufferSize, u32* receivedSize, u32 timeout)
// {
//     if (R_FAILED(IRU_StartRecvTransfer(bufferSize + sizeof(CCRCDCIrdaLargePacketHeader) + 1, 0))) {
//         return -1;
//     }
// 
//     svcWaitSynchronization(recvEvent, timeout * 1000000llu);
//     svcClearEvent(recvEvent);
// 
//     u32 transferCount;
//     if (R_FAILED(IRU_WaitRecvTransfer(&transferCount))) {
//         return -1;
//     }
// 
//     if (!transferCount) {
//         return 0;
//     }
// 
//     u8* buffer = (u8*) sharedMem;
// 
//     // verify magic
//     if (buffer[0] != 0xa5) {
//         printf("IRCReceive: wrong magic (%x)\n", buffer[0]);
//         return -1;
//     }
// 
//     // verify crc
//     if (buffer[transferCount - 1] != crc8(buffer, transferCount - 1)) {
//         printf("IRCReceive: crc mismatch\n");
//         return -1;
//     }
// 
//     CCRCDCIrdaSmallPacketHeader* smallHeader = (CCRCDCIrdaSmallPacketHeader*) buffer;
//     CCRCDCIrdaLargePacketHeader* largeHeader = (CCRCDCIrdaLargePacketHeader*) buffer;
// 
//     void *packetData;
//     uint16_t packetSize;
//     if (smallHeader->isLarge) {
//         packetData = largeHeader + 1;
//         packetSize = __builtin_bswap16(largeHeader->flags) & 0x3fff;
//     } else {
//         packetData = smallHeader + 1;
//         packetSize = smallHeader->dataSize;
//     }
// 
//     // don't count the request size
//     packetSize -= 2;
// 
//     if (packetSize > 0x1fe) {
//         return -1;
//     }
// 
//     *receivedSize = packetSize;
//     memcpy(data, packetData, packetSize);
// 
//     return 0;
// }