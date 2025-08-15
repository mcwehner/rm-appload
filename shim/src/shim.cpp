#include <stdbool.h>
#include <dlfcn.h>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include "shim.h"
#include "fb-shim.h"
#include "input-shim.h"
#include <sys/mman.h>
#include "qtfb-client/common.h"
#include "connection.h"
#include "worldvars.h"

#define FAKE_MODEL "reMarkable 1.0"
#define FILE_MODEL "/sys/devices/soc0/machine"

bool readEnvvarBoolean(const char *name, bool _default) {
    char *value = getenv(name);
    if(value == NULL) {
        return _default;
    }
    return strcmp(value, "1") == 0;
}

void __attribute__((constructor)) __construct () {
    WORLD.shimModel = readEnvvarBoolean("QTFB_SHIM_MODEL", true);
    WORLD.shimInput = readEnvvarBoolean("QTFB_SHIM_INPUT", true);
    WORLD.shimFramebuffer = readEnvvarBoolean("QTFB_SHIM_FB", true);
    WORLD.shimType = FBFMT_RM2FB;
    WORLD.shimInputType = SHIM_INPUT_RM1;
    
    char *fbMode = getenv("QTFB_SHIM_MODE");
    if(fbMode != NULL) {
        if(strcmp(fbMode, "RM2FB") == 0) {
            WORLD.shimType = FBFMT_RM2FB;
        } else if(strcmp(fbMode, "RGB888") == 0) {
            WORLD.shimType = FBFMT_RMPP_RGB888;
        } else if(strcmp(fbMode, "RGBA8888") == 0) {
            WORLD.shimType = FBFMT_RMPP_RGBA8888;
        } else if(strcmp(fbMode, "RGB565") == 0) {
            WORLD.shimType = FBFMT_RMPP_RGB565;
        } else {
            fprintf(stderr, "No such mode supported: %s\n", fbMode);
            abort();
        }
    }

    char *shimMode = getenv("QTFB_SHIM_INPUT_MODE");
    if(shimMode != NULL) {
        if(strcmp(shimMode, "RM1") == 0) {
            WORLD.shimInputType = SHIM_INPUT_RM1;
        } else if(strcmp(shimMode, "RMPP") == 0) {
            WORLD.shimInputType = SHIM_INPUT_RMPP;
        }
    }


    connectShim();
    startPollingThread();
}

void __attribute__((constructor)) __fork_construct () {
    // This should cause the COW-bindings created when fork() is called to be invalidated
    // severing the shim's shared memory between parent and child processes, allowing them
    // to exit separately, without corrupting each other's internal state

    pthread_atfork(NULL, NULL, [](){
        // Manually destroy the page in memory of the child (COW causes the page to be copied)
        memset(&WORLD, 0, sizeof(struct World));
        // ...then reinvoke the constructors
        new (&WORLD) World();
        __construct();
    });
}

void __attribute__((destructor)) __destruct() {
    if(WORLD.clientConnection != NULL) delete WORLD.clientConnection;
}


int spoofModelFD() {
    CERR << "Connected!" << std::endl;
    int modelSpoofFD = memfd_create("Spoof Model Number", 0);
    const char fakeModel[] = FAKE_MODEL;

    if(modelSpoofFD == -1) {
        CERR << "Failed to create memfd for model spoofing" << std::endl;
    }

    if(ftruncate(modelSpoofFD, sizeof(fakeModel) - 1) == -1) {
        CERR << "Failed to truncate memfd for model spoofing: " << errno << std::endl;
    }

    write(modelSpoofFD, fakeModel, sizeof(fakeModel) - 1);
    lseek(modelSpoofFD, 0, 0);
    return modelSpoofFD;
}

static int (*realOpen)(const char *, int, mode_t) = (int (*)(const char *, int, mode_t)) dlsym(RTLD_NEXT, "open");
inline int handleOpen(const char *fileName, int flags, mode_t mode) {
    if(strcmp(fileName, FILE_MODEL) == 0 && WORLD.shimModel) {
        return spoofModelFD();
    }

    int status;
    if(WORLD.shimFramebuffer)
        if((status = fbShimOpen(fileName)) != INTERNAL_SHIM_NOT_APPLICABLE) {
            return status;
        }

    if(WORLD.shimInput)
        if((status = inputShimOpen(fileName, realOpen, flags, mode)) != INTERNAL_SHIM_NOT_APPLICABLE) {
            CERR << "[INPUT] FD ret'd: " << status << std::endl;
            return status;
        }

    return INTERNAL_SHIM_NOT_APPLICABLE;
}

extern "C" int close(int fd) {
    static int (*realClose)(int) = (int (*)(int)) dlsym(RTLD_NEXT, "close");

    int status;
    if(WORLD.shimFramebuffer)
        if((status = fbShimClose(fd)) != INTERNAL_SHIM_NOT_APPLICABLE) {
            return status;
        }
    if(WORLD.shimInput)
        if((status = inputShimClose(fd, realClose)) != INTERNAL_SHIM_NOT_APPLICABLE) {
            return status;
        }

    return realClose(fd);
}

extern "C" int ioctl(int fd, unsigned long request, char *ptr) {
    static int (*realIoctl)(int, unsigned long, ...) = (int (*)(int, unsigned long, ...)) dlsym(RTLD_NEXT, "ioctl");

    int status;
    if(WORLD.shimFramebuffer)
        if((status = fbShimIoctl(fd, request, ptr)) != INTERNAL_SHIM_NOT_APPLICABLE) {
            return status;
        }
    if(WORLD.shimInput)
        if((status = inputShimIoctl(fd, request, ptr, (int (*)(int, unsigned long, char *)) realIoctl)) != INTERNAL_SHIM_NOT_APPLICABLE) {
            return status;
        }

    return realIoctl(fd, request, ptr);
}

extern "C" int __ioctl_time64(int fd, unsigned long request, char *ptr) {
    static int (*realIoctl)(int, unsigned long, ...) = (int (*)(int, unsigned long, ...)) dlsym(RTLD_NEXT, "__ioctl_time64");

    int status;
    if(WORLD.shimFramebuffer)
        if((status = fbShimIoctl(fd, request, ptr)) != INTERNAL_SHIM_NOT_APPLICABLE) {
            return status;
        }
    if(WORLD.shimInput)
        if((status = inputShimIoctl(fd, request, ptr, (int (*)(int, unsigned long, char *)) realIoctl)) != INTERNAL_SHIM_NOT_APPLICABLE) {
            return status;
        }

    return realIoctl(fd, request, ptr);
}


extern "C" int open64(const char *fileName, int flags, mode_t mode) {
    static int (*realOpen64)(const char *, int, mode_t) = (int (*)(const char *, int, mode_t)) dlsym(RTLD_NEXT, "open64");
    int fd;
    if((fd = handleOpen(fileName, flags, mode)) != INTERNAL_SHIM_NOT_APPLICABLE) {
        return fd;
    }

    return realOpen64(fileName, flags, mode);
}

extern "C" int openat(int dirfd, const char *fileName, int flags, mode_t mode) {
    static int (*realOpenat)(int, const char *, int, mode_t) = (int (*)(int, const char *, int, mode_t)) dlsym(RTLD_NEXT, "openat");
    int fd;
    if((fd = handleOpen(fileName, flags, mode)) != INTERNAL_SHIM_NOT_APPLICABLE) {
        return fd;
    }

    return realOpenat(dirfd, fileName, flags, mode);
}

extern "C" int open(const char *fileName, int flags, mode_t mode) {
    int fd;
    if((fd = handleOpen(fileName, flags, mode)) != INTERNAL_SHIM_NOT_APPLICABLE) {
        return fd;
    }

    return realOpen(fileName, flags, mode);
}

extern "C" FILE *fopen(const char *fileName, const char *mode) {
    static FILE *(*realFopen)(const char *, const char *) = (FILE *(*)(const char *, const char *)) dlsym(RTLD_NEXT, "fopen");
    int fd;
    if((fd = handleOpen(fileName, 0, 0)) != INTERNAL_SHIM_NOT_APPLICABLE) {
        return fdopen(fd, mode);
    }

    return realFopen(fileName, mode);
}

#if (__BITS_PER_LONG != 32)
extern "C" FILE *fopen64(const char *fileName, const char *mode) {
    static FILE *(*realFopen)(const char *, const char *) = (FILE *(*)(const char *, const char *)) dlsym(RTLD_NEXT, "fopen64");
    int fd;
    if((fd = handleOpen(fileName, 0, 0)) != INTERNAL_SHIM_NOT_APPLICABLE) {
        return fdopen(fd, mode);
    }

    return realFopen(fileName, mode);
}
#endif
