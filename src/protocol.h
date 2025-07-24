#pragma once

#define MESSAGE_SYSTEM_TERMINATE -1
#define MESSAGE_SYSTEM_NEW_COORDINATOR -2
#define MESSAGE_SYSTEM_LOST_COORDINATOR -3
#define MAX_MESSAGE_LENGTH 10485760 // 10 MiB

struct PacketHeader {
    int type;
    int messageLength;
};
