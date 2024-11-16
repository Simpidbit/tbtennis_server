#ifndef SIMPIDBIT_SIMSKT_H
#define SIMPIDBIT_SIMSKT_H

#include <string>
#include <exception>

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#ifdef _WIN32       // Windows
#include <WinSock2.h>
#else               // Unix
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#endif

#ifdef __linux__    // Linux
#include <sys/poll.h>
#include <sys/epoll.h>
#endif

/*

#ifdef _WIN32
#elif __linux__
#elif __APPLE__
#endif

*/

namespace simpid {

#ifdef _WIN32
using   SocketHandle    = SOCKET;
#else
using   SocketHandle    = int;
#endif

class SocketAbstract {
public:
    SocketHandle        skt;
    char *              recvbuf;
    size_t              recvbuflen;

    int domain;
    int type;
    int protocol;

public:
    SocketAbstract();
    SocketAbstract(
        int     domain      = AF_INET,
        int     type        = SOCK_STREAM,
        int     protocol    = IPPROTO_TCP
        );
    ~SocketAbstract();

    virtual void _abstract() = 0;

    int send(std::string buf);
    int send(const char *buf, size_t length);

    std::string recv(size_t length = 1500);
    std::string recvall();
    int recv(char *buf, size_t length);
};



class Client : public SocketAbstract {
public:
    std::string     ip;
    uint16_t        port;

public:
    Client(
        int     domain      = AF_INET,
        int     type        = SOCK_STREAM,
        int     protocol    = IPPROTO_TCP
        );

    void _abstract() override;

    // Client
    int connect(std::string ip, uint16_t port);
};

class Server : public SocketAbstract {
public:
    int domain;
    int type;
    int protocol;

public:
    Server(
        int     domain      = AF_INET,
        int     type        = SOCK_STREAM,
        int     protocol    = IPPROTO_TCP
        );

    void _abstract() override;

    // Server
    int bind(std::string ip, uint16_t port);
    int listen(int backlog);
    Client accept();

};


namespace SocketException {
class SystemNotReady : public std::exception { const char *what() noexcept; };
class VersionNotSupported : public std::exception { const char *what() noexcept; };
class TooManyProcesses : public std::exception { const char *what() noexcept; };
class EventInProgress : public std::exception { const char *what() noexcept; };
class UnknownError : public std::exception { const char *what() noexcept; };
class VersionNotExist : public std::exception { const char *what() noexcept; };
class InvalidSocket : public std::exception {
public:
    int errcode;
    char * strptr;

    InvalidSocket(int errcode);
    ~InvalidSocket();
    const char *what() noexcept;
};
}

}

namespace spb       = simpid;
namespace Simpid    = simpid;
namespace SP        = simpid;
namespace SPB       = simpid;

#endif
