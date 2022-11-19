#include <iostream>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
#include <arpa/inet.h>
#include "server.h"

void Server::init() {
	// create tcp socket
	if ((_socketFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		perror("socket()");
		exit(1);
	}

	// set address/port reuse
	int opt = 1;
	if (setsockopt(_socketFd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == -1) {
		perror("setsockopt()");
		exit(1);
	}

	// set connection details
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = INADDR_ANY;

	// bind socket fd to port
	if (bind(_socketFd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
		perror("bind()");
		exit(1);
	}

	// listen on port
	if (listen(_socketFd, MAX_LISTENERS) == -1) {
		perror("listen()");
		exit(1);
	}

	// create epoll instance
	if ((_epollFd = epoll_create1(0)) == -1) {
		perror("epoll_create1()");
		exit(1);
	}

	// add socket to epoll interest list
	_setnonblocking(_socketFd);
	_epoll(EPOLL_CTL_ADD, _socketFd, EPOLLIN | EPOLLET);

	std::printf("[INFO] listening on %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
}

void Server::run() {
	char buf[BUFSIZE];
	struct epoll_event events[MAX_EVENTS];

	while (true) {
		// wait for events on the epoll instance
		int n = epoll_wait(_epollFd, events, MAX_EVENTS, EPOLL_TIMEOUT);

		// std::printf("[INFO] epoll_wait() returned %d\n", n);
		for (int i = 0; i < n; i++) {
			int fd = events[i].data.fd;
			if (fd == _socketFd) { // accept new connection
				int clientFd;
				struct sockaddr_in clientAddr;
				socklen_t clientAddrSize = sizeof(clientAddr);
				if ((clientFd = accept(_socketFd, (struct sockaddr *) &clientAddr, &clientAddrSize)) == -1) {
					perror("accept()");
				} else {
					// set fd to non-blocking
					_setnonblocking(clientFd);

					// add fd to interest list
					_epoll(EPOLL_CTL_ADD, clientFd, EPOLLIN | EPOLLOUT | EPOLLET);
					handler->handleConnect(fd);

					std::printf("[WARN] accept connection fd %d\n", clientFd);
				}
			} else if (events[i].events & EPOLLIN) { // read ready
				ssize_t sz = 0, ret;
				while ((ret = read(fd, buf + sz, BUFSIZE - (sz + 1))) > 0) {
					sz += ret;
				};
				buf[sz] = '\0';
				
				if (ret == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) { // read until blocking
					std::printf("[INFO] F%d <-- %s\n", fd, buf);
					handler->handleRequest(fd, buf, sz);
				} else if (ret == 0) { // connection lost
					_epoll(EPOLL_CTL_DEL, fd, 0);
					if (close(fd) == -1) perror("close()");
					handler->handleDisconnect(fd);
					std::printf("[WARN] close fd %d\n", fd);
				} else {
					std::printf("[CRITICAL] ??? bufsize %ld, ret %ld, errno %d\n", sz, ret, errno);
				}

			} else {
				std::printf("[CRITICAL] ??? events[i].events %d\n", events[i].events);
			}
		}
	}
}

/* adds fd to epoll interest list */
void Server::_epoll(int op, int fd, uint32_t events) {
	struct epoll_event ev;
	ev.events = events;
	ev.data.fd = fd;

	if (epoll_ctl(_epollFd, op, fd, &ev) == -1) perror("epoll_ctl()");
}

/* sets fd to nonblocking */
void Server::_setnonblocking(int fd) {
	int flags = fcntl(fd, F_GETFL);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}