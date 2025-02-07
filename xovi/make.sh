cp ../log.h ../management.h ../protocol.h ../management.cpp ../AppLoad.cpp ../AppLoadCoordinator.cpp ../library.cpp ../AppLoad.h ../AppLoadCoordinator.h ../library.h ../AppLibrary.h .
# Do not copy main
qmake6 .
make
