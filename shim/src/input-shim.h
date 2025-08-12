#pragma once
#include <sys/types.h>
#include <poll.h>

#define SHIM_INPUT_RM1 0
#define SHIM_INPUT_RMPP 1

int inputShimOpen(const char *file, int (*realOpen)(const char *name, int flags, mode_t mode), int flags, mode_t mode);
int inputShimClose(int fd, int (*realClose)(int fd));
int inputShimIoctl(int fd, unsigned long request, char *ptr, int (*realFopen)(int fd, unsigned long request, char *ptr));
void startPollingThread();
