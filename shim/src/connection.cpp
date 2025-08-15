#include "qtfb-client/qtfb-client.h"
#include "shim.h"
#include "worldvars.h"

void connectShim(){
    char *fbKey = getenv("QTFB_KEY");
    if(fbKey != NULL) {
        WORLD.shimFramebufferKey = (unsigned int) strtoul(fbKey, NULL, 10);
    }

    CERR << "Connecting to the shim!" << std::endl;
    if(WORLD.shmFD == -1) {
        CERR << "Connecting to the shim step2!" << std::endl;
        WORLD.clientConnection = new qtfb::ClientConnection(WORLD.shimFramebufferKey, WORLD.shimType, {}, false);
        WORLD.shmFD = WORLD.clientConnection->shmFD;
        WORLD.shmMemory = WORLD.clientConnection->shm;
    }
}
