#include <sys/select.h>
#include "input-shim.h"
#include "shim.h"
#include <algorithm>
#include <deque>
#include <map>
#include <cstring>
#include <condition_variable>
#include <fcntl.h>
#include <asm/ioctl.h>
#include <linux/input-event-codes.h>
#include <linux/input.h>
#include <vector>
#include <thread>
#include <unistd.h>
#include <dlfcn.h>
#include <poll.h>

#include "qtfb-client/qtfb-client.h"

#define DEV_NULL "/dev/null"

#define RM1_TOUCHSCREEN "/dev/input/event2"
#define RM1_BUTTONS "/dev/input/event1"
#define RM1_DIGITIZER "/dev/input/event0"

#define RMPP_TOUCHSCREEN "/dev/input/event3"
#define RMPP_DIGITIZER "/dev/input/event2"

#define RM1_MAX_DIGI_X 20967
#define RM1_MAX_DIGI_Y 15725
#define RMPP_MAX_DIGI_X 11180
#define RMPP_MAX_DIGI_Y 15340

#define RM1_MAX_TOUCH_X 767
#define RM1_MAX_TOUCH_Y 1023
#define RM1_MAX_PRESSURE 4095

#define RM1_MAX_TOUCH_MAJOR 255
#define RM1_MAX_TOUCH_MINOR 255
#define RM1_MIN_ORIENTATION -127
#define RM1_MAX_ORIENTATION  127

#define RMPP_MAX_TOUCH_X 2064
#define RMPP_MAX_TOUCH_Y 2832
#define RMPP_MAX_PRESSURE 255

extern qtfb::ClientConnection *clientConnection;
extern int shimInputType;

struct TouchSlotState {
    int x, y;
};

std::map<int, TouchSlotState> touchStates;

#define QUEUE_TOUCH 1
#define QUEUE_PEN 2
#define QUEUE_BUTTONS 3

struct PerFDEventQueue {
    int queueType;
    int queueRead;
    int queueWrite;

    PerFDEventQueue(int e, int r, int w): queueType(e), queueRead(r), queueWrite(w) {}
};
static std::map<int, PerFDEventQueue> eventQueue;
static struct {
    int pipeRead, pipeWrite;
} nullPipe;

static struct input_event evt(unsigned short type, unsigned short code, int value) {
#if (__BITS_PER_LONG != 32)
    timeval time;
    gettimeofday(&time, NULL);
    struct input_event x = {
        .time = time,
        .type = type,
        .code = code,
        .value = value,
    };
#else
    struct input_event x = {
        .type = type,
        .code = code,
        .value = value,
    };
#endif
    return x;
}

static int mapKey(int x) {
    switch(x) {
        case INPUT_BTN_X_LEFT: return KEY_LEFT;
        case INPUT_BTN_X_RIGHT: return KEY_RIGHT;
        case INPUT_BTN_X_HOME: return KEY_HOME;
    }
    return 0;
}

static void pushToAll(int queueType, struct input_event evt) {
    for(auto &ref : eventQueue) {
        if(ref.second.queueType == queueType) {
            write(ref.second.queueWrite, &evt, sizeof(evt));
        }
    }
}

static void pollInputUpdates() {
    qtfb::ServerMessage message;
    if(clientConnection) {
        if(clientConnection->pollServerPacket(message) && message.type == MESSAGE_USERINPUT) {
            // Did we get a packet?
            char state_a;

            int xTranslate, yTranslate, dTranslate;
            switch(shimInputType) {
                case SHIM_INPUT_RM1:
                    switch(message.userInput.inputType & 0xF0) {
                        case INPUT_TOUCH_PRESS:
                            xTranslate = RM1_MAX_TOUCH_X - ((message.userInput.x * RM1_MAX_TOUCH_X) / (int) clientConnection->width());
                            yTranslate = RM1_MAX_TOUCH_Y - ((message.userInput.y * RM1_MAX_TOUCH_Y) / (int) clientConnection->height());
                            break;
                        case INPUT_PEN_PRESS:
                            xTranslate = RM1_MAX_DIGI_X - ((message.userInput.y * RM1_MAX_DIGI_X) / clientConnection->height());
                            yTranslate = (message.userInput.x * RM1_MAX_DIGI_Y) / clientConnection->width();
                            dTranslate = (message.userInput.d * 4096) / 100;
                            break;
                    }
                    break;
                case SHIM_INPUT_RMPP:
                    switch(message.userInput.inputType & 0xF0) {
                        case INPUT_TOUCH_PRESS:
                            xTranslate = ((message.userInput.x * RMPP_MAX_TOUCH_X) / (int) clientConnection->width());
                            yTranslate = ((message.userInput.y * RMPP_MAX_TOUCH_Y) / (int) clientConnection->height());
                            break;
                        case INPUT_PEN_PRESS:
                            xTranslate = (message.userInput.x * RMPP_MAX_DIGI_X) / clientConnection->width();
                            yTranslate = (message.userInput.y * RMPP_MAX_DIGI_Y) / clientConnection->height();
                            dTranslate = (message.userInput.d * 255) / 100;
                            break;
                    }
                    break;
            }

            CERR << "[QTFB SHIM INPUT]: " << (int) message.userInput.inputType << ", " << message.userInput.x << ", " << message.userInput.y << std::endl;
            switch(message.userInput.inputType) {
                case INPUT_TOUCH_PRESS:
                    state_a = 1;
                    pushToAll(QUEUE_TOUCH, evt(EV_ABS, ABS_MT_TRACKING_ID, 50));
                    goto sendpos;
                case INPUT_TOUCH_RELEASE:
                    state_a = 0;
                    pushToAll(QUEUE_TOUCH, evt(EV_ABS, ABS_MT_TRACKING_ID, -1));
                    sendpos:
                    if(state_a != 0) {
                        pushToAll(QUEUE_TOUCH, evt(EV_ABS, ABS_MT_POSITION_X, xTranslate));
                        pushToAll(QUEUE_TOUCH, evt(EV_ABS, ABS_MT_POSITION_Y, yTranslate));
                    }
                    if(state_a == 1 || state_a == 0) {
                        pushToAll(QUEUE_TOUCH, evt(EV_KEY, BTN_TOUCH, state_a));
                    }
                    pushToAll(QUEUE_TOUCH, evt(EV_SYN, SYN_REPORT, 0));
                    break;
                case INPUT_TOUCH_UPDATE:{
                    state_a = 2;
                    goto sendpos;
                    break;
                }
                case INPUT_PEN_PRESS:
                    pushToAll(QUEUE_PEN, evt(EV_KEY, BTN_TOOL_PEN, 1));
                    pushToAll(QUEUE_PEN, evt(EV_KEY, BTN_TOUCH, 1));
                    goto pen_fall;
                case INPUT_PEN_RELEASE:
                    pushToAll(QUEUE_PEN, evt(EV_KEY, BTN_TOOL_PEN, 1));
                    pushToAll(QUEUE_PEN, evt(EV_KEY, BTN_TOUCH, 0));
                    goto pen_fall;
                case INPUT_PEN_UPDATE:
                    pushToAll(QUEUE_PEN, evt(EV_KEY, BTN_TOOL_PEN, 1));
                pen_fall:
                    pushToAll(QUEUE_PEN, evt(EV_ABS, ABS_X, xTranslate));
                    pushToAll(QUEUE_PEN, evt(EV_ABS, ABS_Y, yTranslate));
                    pushToAll(QUEUE_PEN, evt(EV_ABS, ABS_PRESSURE, dTranslate));
                    pushToAll(QUEUE_PEN, evt(EV_SYN, SYN_REPORT, 0));
                    break;
                case INPUT_BTN_PRESS:
                    pushToAll(QUEUE_BUTTONS, evt(EV_KEY, mapKey(message.userInput.x), 1));
                    pushToAll(QUEUE_BUTTONS, evt(EV_SYN, SYN_REPORT, 0));
                    break;
                case INPUT_BTN_RELEASE:
                    pushToAll(QUEUE_BUTTONS, evt(EV_KEY, mapKey(message.userInput.x), 0));
                    pushToAll(QUEUE_BUTTONS, evt(EV_SYN, SYN_REPORT, 0));
                    break;
                default: break;
            }
        }
    }
}

static std::thread pollingThread;
static bool pollingThreadRunning;

static void killPollingThread() {
    pollingThreadRunning = false;
    // pollingThreadRunning.join();
}

void startPollingThread() {
    pipe((int*) &nullPipe);
    pollingThreadRunning = true;
    pollingThread = std::thread([&]() {
        while(pollingThreadRunning) {
            pollInputUpdates();
        }
    });

    atexit(killPollingThread);
}

static int createInEventMap(int type, int flags) {
    int _pipe[2];
    if(pipe2(_pipe, flags & O_NONBLOCK) == -1){
        CERR << "Failed to create pipe: " << errno << std::endl;
        abort();
    }
    CERR << "Create evqueue pipe r:" << _pipe[0] << ", w:" << _pipe[1] << std::endl;
    eventQueue.try_emplace(_pipe[0], type, _pipe[0], _pipe[1]);
    return _pipe[0];
}

int inputShimOpen(const char *file, int (*realOpen)(const char *name, int flags, mode_t mode), int flags, mode_t mode) {
    if(shimInputType == SHIM_INPUT_RM1) {
        if(strcmp(file, RM1_DIGITIZER) == 0) {
            int fd = createInEventMap(QUEUE_PEN, flags);
            CERR << "Open digitizer " << fd << std::endl;
            return fd;
        }

        if(strcmp(file, RM1_TOUCHSCREEN) == 0) {
            int fd = createInEventMap(QUEUE_TOUCH, flags);
            CERR << "Open touchscreen " << fd << std::endl;
            return fd;
        }

        if(strcmp(file, RM1_BUTTONS) == 0) {
            int fd = createInEventMap(QUEUE_BUTTONS, flags);
            CERR << "Open buttons " << fd << std::endl;
            return fd;
        }
    } else if(shimInputType == SHIM_INPUT_RMPP) {
        if(strcmp(file, RMPP_DIGITIZER) == 0) {
            int fd = createInEventMap(QUEUE_PEN, flags);
            CERR << "Open digitizer " << fd << std::endl;
            return fd;
        }

        if(strcmp(file, RMPP_TOUCHSCREEN) == 0) {
            int fd = createInEventMap(QUEUE_TOUCH, flags);
            CERR << "Open touchscreen " << fd << std::endl;
            return fd;
        }

    }

    return INTERNAL_SHIM_NOT_APPLICABLE;
}

int inputShimClose(int fd, int (*realClose)(int)) {
    CERR << "Shim close " << fd << std::endl;
    auto position = eventQueue.find(fd);
    if(position != eventQueue.end()) {
        realClose(position->second.queueRead);
        realClose(position->second.queueWrite);
        eventQueue.erase(position);
        return 0;
    }

    return INTERNAL_SHIM_NOT_APPLICABLE;
}

static int fakeOrOverrideAbsInfo(
    int fd,
    unsigned long request,
    char *ptr,
    int (*realIoctl)(int, unsigned long, char*),
    int minVal,
    int maxVal,
    int fuzzVal,
    int flatVal,
    int resolutionVal)
{
    // Disregard original return value.
    struct input_absinfo *absinfo = reinterpret_cast<struct input_absinfo*>(ptr);
    std::memset(absinfo, 0, sizeof(absinfo));

    // realIoctl(fd, request, ptr);
    absinfo->minimum    = minVal;
    absinfo->maximum    = maxVal;
    absinfo->fuzz       = fuzzVal;
    absinfo->flat       = flatVal;
    absinfo->resolution = resolutionVal;
    return 0;
}

#define BITS_PER_LONG (sizeof(long) * 8)
#define NBITS(x) ((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x)  ((x)%BITS_PER_LONG)
#define BIT(x)  (1ULL<<OFF(x))
#define LONG(x) ((x)/BITS_PER_LONG)
#define SETBIT(bit, array)  (array[LONG(bit)] |= BIT(bit))

#define IS_MATCHING_IOCTL(dir, type, nr) ((request & ~(_IOC_SIZEMASK << _IOC_SIZESHIFT)) == (_IOC(dir, type, nr, 0)))
#define IS_MATCHING_IOCTL_S(dir, type, nr, size) (request == (_IOC(dir, type, nr, size)))
int inputShimIoctl(int fd, unsigned long request, char *ptr, int (*realIoctl)(int fd, unsigned long request, char *ptr)) {
    auto position = eventQueue.find(fd);
    if(position == eventQueue.end()) return INTERNAL_SHIM_NOT_APPLICABLE;
    const auto &ref = eventQueue.at(fd);
    int ioctlInternalSize = _IOC_SIZE(request);

    unsigned cmdDir  = _IOC_DIR(request);
    unsigned cmdType = _IOC_TYPE(request);
    unsigned cmdNr   = _IOC_NR(request);
    unsigned cmdSize = _IOC_SIZE(request);

    if (ref.queueType == QUEUE_TOUCH) {
        CERR << "Touchscreen IOCTL: " << request << std::endl;
        int status;

        if (IS_MATCHING_IOCTL_S(_IOC_READ, 'E', 0x40 + ABS_MT_POSITION_X, sizeof(input_absinfo))) {
            return fakeOrOverrideAbsInfo(fd, request, ptr, realIoctl,
                                        0, RM1_MAX_TOUCH_X,
                                        100, 0, 0);
        }
        if (IS_MATCHING_IOCTL_S(_IOC_READ, 'E', 0x40 + ABS_MT_POSITION_Y, sizeof(input_absinfo))) {
            return fakeOrOverrideAbsInfo(fd, request, ptr, realIoctl,
                                        0, RM1_MAX_TOUCH_Y,
                                        100, 0, 0);
        }
        if (IS_MATCHING_IOCTL_S(_IOC_READ, 'E', 0x40 + ABS_MT_ORIENTATION, sizeof(input_absinfo))) {
            struct input_absinfo *absinfo = reinterpret_cast<struct input_absinfo*>(ptr);
            std::memset(absinfo, 0, sizeof(absinfo));
            // int status = realIoctl(fd, request, ptr);
            absinfo->minimum = RM1_MIN_ORIENTATION;
            absinfo->maximum = RM1_MAX_ORIENTATION;
            return 0;
        }

        if(IS_MATCHING_IOCTL(_IOC_READ, 'E', 0x6)) {
            // Get Name
            CERR << "Get device name" << std::endl;
            strncpy(ptr, "cyttsp5_mt", cmdSize);
            return 0;
        }

        // pretend like we support everything the RM1 supports
        if (cmdDir == _IOC_READ && cmdType == 'E' && cmdNr == 0x20) {
            // int status = realIoctl(fd, request, ptr);
            // if (status < 0) return status;

            unsigned long *bits = (unsigned long*) ptr;
            SETBIT(EV_ABS, bits);
            SETBIT(EV_REL, bits);
            return 0;
        }

        if (cmdDir == _IOC_READ && cmdType == 'E' && cmdNr == (0x20 + EV_ABS)) {
            // int status = realIoctl(fd, request, ptr);
            // if (status < 0) return status;

            unsigned long *bits = (unsigned long*) ptr;

            SETBIT(ABS_MT_POSITION_X, bits);
            SETBIT(ABS_MT_POSITION_Y, bits);
            SETBIT(ABS_MT_PRESSURE, bits);
            SETBIT(ABS_MT_TOUCH_MAJOR, bits);
            SETBIT(ABS_MT_TOUCH_MINOR, bits);
            SETBIT(ABS_MT_ORIENTATION, bits);
            SETBIT(ABS_MT_SLOT, bits);
            SETBIT(ABS_MT_TOOL_TYPE, bits);
            SETBIT(ABS_MT_TRACKING_ID, bits);

            return 0;
        }
        return 0;
    }

    if(ref.queueType == QUEUE_PEN) {
        CERR << "Digitizer IOCTL: " << request << std::endl;

        int status;

        if(IS_MATCHING_IOCTL(_IOC_READ, 'E', 0x6)) {
            // Get Name
            CERR << "Get device name" << std::endl;
            strncpy(ptr, "Wacom I2C Digitizer", cmdSize);
            return 0;
        }

        if (IS_MATCHING_IOCTL_S(_IOC_READ, 'E', 0x40 + ABS_X, sizeof(input_absinfo))) {
            return fakeOrOverrideAbsInfo(fd, request, ptr, realIoctl,
                                        0, RM1_MAX_DIGI_X,
                                        0, 0, 0);
        }
        if (IS_MATCHING_IOCTL_S(_IOC_READ, 'E', 0x40 + ABS_Y, sizeof(input_absinfo))) {
            return fakeOrOverrideAbsInfo(fd, request, ptr, realIoctl,
                                        0, RM1_MAX_DIGI_Y,
                                        0, 0, 0);
        }
        return 0;
    }

    if (ref.queueType == QUEUE_BUTTONS) {
        CERR << "Buttons IOCTL: " << request << std::endl;
        int status;

        if(IS_MATCHING_IOCTL(_IOC_READ, 'E', 0x6)) {
            // Get Name
            strncpy(ptr, "gpio_buttons", cmdSize);
            return 0;
        }

        if (cmdDir == _IOC_READ && cmdType == 'E' && cmdNr == 0x20) {
            // int status = realIoctl(fd, request, ptr);
            // if (status < 0) return status;

            unsigned long *bits = (unsigned long*) ptr;
            SETBIT(EV_SYN, bits);
            SETBIT(EV_KEY, bits);

            return 0;
        }

        if (cmdDir == _IOC_READ && cmdType == 'E' && cmdNr == (0x20 + EV_KEY)) {
            // int status = realIoctl(fd, request, ptr);
            // if (status < 0) return status;

            unsigned long *bits = (unsigned long*) ptr;

            SETBIT(KEY_HOME, bits);
            SETBIT(KEY_LEFT, bits);
            SETBIT(KEY_RIGHT, bits);
            SETBIT(KEY_WAKEUP, bits);
            SETBIT(KEY_POWER, bits);

            return 0;
        }
        return 0;
    }

    CERR << "Unknown shim ioctl!" << std::endl;

    return INTERNAL_SHIM_NOT_APPLICABLE;
}
