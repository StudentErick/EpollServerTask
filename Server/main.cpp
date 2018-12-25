#include <iostream>
#include "MyServer.h"
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cout << "usage:" << argv[0] << " <port of server>\n";
        return 0;
    }

    int port = atoi(argv[1]);
    if (port < MIN_PORT || port > MAX_PORT) {
        std::cout << "port error\n";
        return 0;
    }

    MyServer myServer(10);
    std::cout << "set server ok...\n\n";
    if (!myServer.listen(port)) {
        std::cout << "listen error !";
        return 0;
    }
    std::cout << "start listening on port: " << port << std::endl;
    if (!myServer.startService()) {
        std::cout << "start service error\n";
        return 0;
    }
    std::cout << "service on port " << port << std::endl;

    return 0;
}