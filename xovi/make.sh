source ~/Tools/remarkable-toolchain/environment-setup-cortexa53-crypto-remarkable-linux
cp ../log.h ../management.h ../protocol.h ../management.cpp ../AppLoad.cpp ../AppLoadCoordinator.cpp ../library.cpp ../AppLoad.h ../AppLoadCoordinator.h ../library.h ../AppLibrary.h .
# Do not copy main
python3 $XOVI_HOME/util/xovigen.py --cpp -o output appload.xovi
mv output/xovi.c output/main.cpp
qmake6 .
make -j`nproc`
