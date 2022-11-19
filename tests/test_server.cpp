#define PORT 8000
#define NUM_PROCS 5
#define BUFFER_SIZE 512

#include <iostream>
#include <sys/socket.h>
#include <sys/wait.h>
#include <cstring>
#include <arpa/inet.h>
// #include <thread>
// #include <chrono>
#include <csignal>
#include <assert.h>

void * doWork(void * arg);

struct Args {
	int id;
};

pid_t PIDS[NUM_PROCS];

void sighandler(int signum) {
	for (int i = 0; i < NUM_PROCS; i++) {
		kill(PIDS[i], SIGKILL);
	}
}

int main() {
	for (int i = 0; i < NUM_PROCS; i++) {
		if ((PIDS[i] = fork()) == 0) {
			struct Args * arg = new struct Args();
			arg->id = i;
			doWork(arg);
			exit(0);
		}
	}

	signal(SIGTERM, sighandler);
	signal(SIGINT, sighandler);

	while (wait(NULL) > 0);
}

// int main() {

// 	// create thread pool
// 	pthread_t workers[NUM_PROCS];
// 	for (int i = 0; i < NUM_PROCS; i++) {
// 		struct Args * arg = new struct Args();
// 		arg->id = i;
// 		if (pthread_create(&workers[i], NULL, &doWork, arg) == -1) {
// 			perror("pthread_create()");
// 			exit(1);
// 		}
// 	}

// 	for (int i = 0; i < NUM_PROCS; i++) {
// 		if (pthread_join(workers[i], NULL) == -1) {
// 			perror("pthread_join()");
// 			exit(1);
// 		}
// 	}
// }

void * doWork(void * args) {
	// create tcp socket
	struct Args * arg = (struct Args *) args;
	int socketFd;
	if ((socketFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		perror("socket()");
		exit(1);
	}

	// set connection details
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	// connect
	if (connect(socketFd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
		perror("connect()");
		exit(1);
	}

	char buffer[BUFFER_SIZE + 1];
	int i = 0;

	printf("[INFO] P%d socket %d\n", arg->id, socketFd);

	srand(arg->id);
	
	while (true) {
		buffer[0] = 97 + i;
		buffer[1] = '\0';

		if (write(socketFd, buffer, 2) < 0) {
			perror("write()");
			exit(1);
		};

		printf("[INFO] P%d --> %s\n", arg->id, buffer);

		if (read(socketFd, buffer, BUFFER_SIZE) < 0) {
			perror("read()");
			exit(1);
		};

		printf("[INFO] P%d <-- %s\n", arg->id, buffer);

		assert(buffer[0] == i + 97 - 32);

		i = (i + 1) % 26;

		sleep((double) rand() / RAND_MAX);
	}
}

