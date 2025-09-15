#include <3ds.h>
#include <3ds/services/ir.h>

u8* IRCInit(u8 bitrate);
int IRCShutdown(void);
int IRCSend(u8* data, u32 size);
int IRCReceive(u32* actualReadSize, u32 readSize, u32 timeout, u8* out);
