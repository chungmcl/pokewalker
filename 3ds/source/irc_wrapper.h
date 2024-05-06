#include <3ds.h>
#include <3ds/services/ir.h>

u8* IRCInit(u8 bitrate);
int IRCShutdown(void);
int IRCSend(u8* data, u32 size);
int IRCReceive(u32* actualReadSize, u32 readSize, u32 timeout, u8* out);

typedef struct PACKED CCRCDCIrdaLargePacketHeader
{
   //! Magic value (needs to be set to 0xa5)
   uint8_t magic;
   //! ID of the session (gets filled out by the DRC before transmitting the packet)
   uint8_t sessionId;
   /* can't really use a nice bitfield here due to endianness */
   //! Size of the actual data + isLarge + unk
   uint16_t flags;
   //! Amount of data which should be received from the other device
   uint16_t receiveSize;
} CCRCDCIrdaLargePacketHeader;