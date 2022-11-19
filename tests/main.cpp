#include <unistd.h>
#include "server.h"

class Handler : public Server::Handler {
  void handleConnect(int fd) override {};

  void handleRequest(int fd, char* buf, size_t bufsz) override {
    for (int i = 0; i < bufsz; i++)
      buf[i] -= 32;
    write(fd, buf, bufsz);
  };

  void handleDisconnect(int fd) override {};
};

int main() {
  Server s;
  s.handler = new Handler();
  s.init();
  s.run();
}