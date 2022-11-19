#define MAX_EVENTS 5
#define BUFSIZE 512
#define MAX_LISTENERS 16
#define EPOLL_TIMEOUT 5000
#define PORT 8000

class Server {
	public:
		Server() = default;
		~Server() = default;
		Server(const Server&) = delete;
		Server& operator=(const Server&) = delete;
		Server(Server&&) = delete;
		Server& operator=(Server&&) = delete;

		class Handler {
			public:
				/* invoked when request is received on @fd */
				virtual void handleRequest(int fd, char* buf, size_t bufsz) = 0;

				/* invoked when new connection is accepted on @fd */
				virtual void handleConnect(int fd) = 0;

				/* invoked when connection from @fd is closed */
				virtual void handleDisconnect(int fd) = 0;
		};

		Handler* handler;

        void init();
		void run();

	private:
        int _socketFd, _epollFd;
		void _epoll(int op, int fd, uint32_t events);
		void _setnonblocking(int fd);
};