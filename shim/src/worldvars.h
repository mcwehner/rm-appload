#include "qtfb-client/qtfb-client.h"
#include <thread>
#include <map>

struct TouchSlotState {
    int x, y;
};

struct PerFDEventQueue {
    int queueType;
    int queueRead;
    int queueWrite;

    PerFDEventQueue(int e, int r, int w): queueType(e), queueRead(r), queueWrite(w) {}
};

extern struct World {
    /* Connection */
    qtfb::FBKey shimFramebufferKey;
    uint8_t shimType;

    qtfb::ClientConnection *clientConnection = NULL;
    void *shmMemory = NULL;
    int shmFD = -1;

    /* Config */
    bool shimModel;
    bool shimInput;
    bool shimFramebuffer;
    int shimInputType;

    /* Inputs */
    std::map<int, TouchSlotState> touchStates;
    std::map<int, PerFDEventQueue> eventQueue;
    struct {
        int pipeRead, pipeWrite;
    } nullPipe;

    std::thread pollingThread;
    bool pollingThreadRunning;
} WORLD;
