#include "simskt.h"
/*
   linux下程序的返回值:
    1 - socket() 出错
    2 - bind() 出错
    3 - listen() 出错
    4 - accept() 出错
    5 - connect() 出错
    6 - send() 出错
    7 - recv() 出错
*/

#include "simskt.h"

// 出错时是否抛出异常
bool IF_THROW = true;

#define DEFINE_EXCEPTION_WHAT(EXCEPTION_NAME, WHATSTR) \
const char * simpid::SocketException::EXCEPTION_NAME::what() noexcept { return WHATSTR; }
DEFINE_EXCEPTION_WHAT(SystemNotReady, "System was not ready, check your system settings, win32 library or environment configuration. Error code: 10091.")
DEFINE_EXCEPTION_WHAT(VersionNotSupported, "Version was not supported by your network library.")
DEFINE_EXCEPTION_WHAT(TooManyProcesses, "Too many processes, close some of them and try again.")
DEFINE_EXCEPTION_WHAT(EventInProgress, "Event in progress, the event you requested had been in progress.")
DEFINE_EXCEPTION_WHAT(UnknownError, "Unknown error, I don\'t know what happened!")
DEFINE_EXCEPTION_WHAT(VersionNotExist, "Version not exists. Make sure your version is valid. Maybe you can try to modify the variable \"version_required\" in \"simskt.cpp\".")

simpid::SocketException::InvalidSocket::InvalidSocket(int errcode) : errcode(errcode) {}
const char * simpid::SocketException::InvalidSocket::what() noexcept
{
    std::string tmpstr = (std::string)"Invalid socket: error code = " + std::to_string(this->errcode);
    this->strptr = new char(tmpstr.size() + 1);
    memcpy(this->strptr, tmpstr.c_str(), tmpstr.size());
    this->strptr[tmpstr.size()] = '\0';
    return this->strptr;
}
simpid::SocketException::InvalidSocket::~InvalidSocket()
{
    delete this->strptr;
}

simpid::SocketAbstract::SocketAbstract()
{
    this->recvbuflen = BUFSIZ + 1;
    this->recvbuf = (char *)malloc(this->recvbuflen);
    memset(this->recvbuf, 0, this->recvbuflen);
}

simpid::SocketAbstract::SocketAbstract( int domain,
                                       int type,
                                       int protocol)
    : domain(domain), type(type), protocol(protocol)
{
#ifdef _WIN32
    WORD version_required = MAKEWORD(2, 2);
    WSADATA sock_msg;

    switch(WSAStartup(version_required, &sock_msg)) {
    case WSASYSNOTREADY:
        WSACleanup();
        if (IF_THROW) throw simpid::SocketException::SystemNotReady();
        break;
    case WSAVERNOTSUPPORTED:
        WSACleanup();
        if (IF_THROW) throw simpid::SocketException::VersionNotSupported();
        break;
    case WSAEPROCLIM:
        WSACleanup();
        if (IF_THROW) throw simpid::SocketException::TooManyProcesses();
        break;
    case WSAEINPROGRESS:
        WSACleanup();
        if (IF_THROW) throw simpid::SocketException::EventInProgress();
        break;
    }

    if (HIBYTE(sock_msg.wVersion) != 2
        || LOBYTE(sock_msg.wVersion) != 2) {
        WSACleanup();
        if (IF_THROW) throw simpid::SocketException::VersionNotExist();
    }
#endif

    this->skt = socket(domain, type, protocol);

#ifdef _WIN32
    if (this->skt == INVALID_SOCKET) {
        WSACleanup();
        if (IF_THROW) throw simpid::SocketException::InvalidSocket(WSAGetLastError());
    }
#else
    if (this->skt == -1) {
        fprintf(stderr, "errno: %d\n", errno);
        fprintf(stderr, "An error threw by socket(): %s\n", strerror(errno));
        exit(1);
    }
#endif
    this->recvbuflen = BUFSIZ + 1;
    this->recvbuf = (char *)malloc(this->recvbuflen);
    memset(this->recvbuf, 0, this->recvbuflen);
}

simpid::SocketAbstract::~SocketAbstract()
{
#ifdef _WIN32
    closesocket(this->skt);
    WSACleanup();
#else
    close(this->skt);
#endif
}


int
simpid::SocketAbstract::send(std::string buf)
{
    return this->send(buf.c_str(), buf.size());
}

int
simpid::SocketAbstract::send(const char *buf, size_t length)
{
    int tmp = 0;
    tmp = ::send(this->skt, buf, length, 0);
#ifdef _WIN32
    if (tmp == SOCKET_ERROR) {
        closesocket(this->skt);
        if (IF_THROW) throw simpid::SocketException::InvalidSocket(WSAGetLastError());
    }
#else
    if (tmp < 0) {
        close(this->skt);
        fprintf(stderr, "errno: %d\n", errno);
        fprintf(stderr, "An error threw by connect(): %s\n", strerror(errno));
        exit(6);
    }
#endif
    return tmp;
}

std::string
simpid::SocketAbstract::recv(size_t length)
{
    size_t n = this->recv(this->recvbuf, this->recvbuflen - 1);
    if (n >= 0) this->recvbuf[n] = '\0';
    return std::string(this->recvbuf);
}

std::string
simpid::SocketAbstract::recvall()
{
    std::string msg;

    char tmp[BUFSIZ + 1];
    while (this->recv(tmp, BUFSIZ) == BUFSIZ) {
        msg = msg + (std::string)tmp;
        memset(tmp, 0, BUFSIZ + 1);
    }
    msg = msg + (std::string)tmp;
    memset(tmp, 0, BUFSIZ + 1);

    return msg;
}

int
simpid::SocketAbstract::recv(char * buf, size_t length)
{
    // length 默认是比buf的长度要小的
    int tmp = 0;
    memset(buf, 0, length);
    tmp = ::recv(this->skt, buf, length, 0);
    if (tmp >= 0) buf[tmp] = '\0';
#ifdef _WIN32
    if (tmp == SOCKET_ERROR) {
        closesocket(this->skt);
        WSACleanup();
        if (IF_THROW) throw simpid::SocketException::InvalidSocket(WSAGetLastError());
    }
#else
    if (tmp < 0) {
        close(this->skt);
        fprintf(stderr, "errno: %d\n", errno);
        fprintf(stderr, "An error threw by recv(): %s\n", strerror(errno));
        exit(7);
    }
#endif
    return tmp;
}


simpid::Server::Server(int domain, int type, int protocol)
    : simpid::SocketAbstract(domain, type, protocol)
{
}


void simpid::Server::_abstract()
{
}

int
simpid::Server::bind(std::string ip, uint16_t port)
{
    int tmp;

#ifdef _WIN32
    SOCKADDR_IN servaddr;
#else
    struct sockaddr_in servaddr;
#endif
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(ip.c_str());
    servaddr.sin_port = htons(port);

    tmp = 0;
    tmp = ::bind(this->skt, (struct sockaddr*)&servaddr, sizeof(servaddr));

#ifdef _WIN32
    if (tmp == SOCKET_ERROR) {
        closesocket(this->skt);
        WSACleanup();
        if (IF_THROW) throw simpid::SocketException::InvalidSocket(WSAGetLastError());
    }
#else
    if (tmp == -1) {
        close(this->skt);
        fprintf(stderr, "errno: %d\n", errno);
        fprintf(stderr, "An error threw by bind(): %s\n", strerror(errno));
        exit(2);
    }
#endif
    return tmp;
}

int
simpid::Server::listen(int backlog)
{
    int tmp = 0;
    tmp = ::listen(this->skt, backlog);
#ifdef _WIN32
    if (tmp == SOCKET_ERROR) {
        closesocket(this->skt);
        WSACleanup();
        if (IF_THROW) throw simpid::SocketException::InvalidSocket(WSAGetLastError());
    }
#else
    if (tmp < 0) {
        close(this->skt);
        fprintf(stderr, "errno: %d\n", errno);
        fprintf(stderr, "An error threw by listen(): %s\n", strerror(errno));
        exit(3);
    }
#endif
    return tmp;
}

simpid::Client
simpid::Server::accept()
{
#ifdef _WIN32
    SOCKADDR_IN cliaddr;
    int cliaddr_len;
#else
    sockaddr_in cliaddr;
    socklen_t cliaddr_len;
#endif
    Client cli;
    cli.skt = ::accept(this->skt, (struct sockaddr*)&cliaddr, &cliaddr_len);
#ifdef _WIN32
    if (cli.skt == INVALID_SOCKET) {
        closesocket(this->skt);
        WSACleanup();
        if (IF_THROW) throw simpid::SocketException::InvalidSocket(WSAGetLastError());
    }
#else
    if (cli.skt < 0) {
        ::close(this->skt);
        fprintf(stderr, "errno: %d\n", errno);
        fprintf(stderr, "An error threw by accept(): %s\n", strerror(errno));
        exit(4);
    }
#endif
    cli.ip = inet_ntoa(cliaddr.sin_addr);
    cli.port = cliaddr.sin_port;
    return cli;
}

simpid::Client::Client(int domain, int type, int protocol)
    : simpid::SocketAbstract(domain, type, protocol)
{
}


void simpid::Client::_abstract()
{
}

int
simpid::Client::connect(std::string ip, uint16_t port)
{
    int tmp;
#ifdef _WIN32
    SOCKADDR_IN servaddr;
#else
    struct sockaddr_in servaddr;
#endif
    servaddr.sin_family = this->domain;
    servaddr.sin_addr.s_addr = inet_addr(ip.c_str());
    servaddr.sin_port = htons(port);

    tmp = 0;
    tmp = ::connect(this->skt, (struct sockaddr*)&servaddr, sizeof(servaddr));

#ifdef _WIN32
    if (tmp == SOCKET_ERROR) {
        closesocket(this->skt);
        WSACleanup();
        if (IF_THROW) throw simpid::SocketException::InvalidSocket(WSAGetLastError());
    }
#else
    if (tmp == -1) {
        ::close(this->skt);
        fprintf(stderr, "errno: %d\n", errno);
        fprintf(stderr, "An error threw by connect(): %s\n", strerror(errno));
        exit(5);
    }
#endif
    return tmp;
}
