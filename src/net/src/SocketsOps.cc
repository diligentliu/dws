#include "SocketsOps.h"

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/uio.h>  // readv
#include <unistd.h>

#include <cerrno>
#include <cstdio>  // snprintf

#include "Endian.h"
#include "Logging.h"
#include "Types.h"

namespace dws::net {

namespace {

using SA = struct sockaddr;

#if VALGRIND || defined(NO_ACCEPT4)
void setNonBlockAndCloseOnExec(int sockfd) {
    // non-block
    int flags = ::fcntl(sockfd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    int ret = ::fcntl(sockfd, F_SETFL, flags);
    // FIXME check

    // close-on-exec
    flags = ::fcntl(sockfd, F_GETFD, 0);
    flags |= FD_CLOEXEC;
    ret = ::fcntl(sockfd, F_SETFD, flags);
    // FIXME check

    (void)ret;
}
#endif

}  // namespace

namespace sockets {

const SA *sockaddr_cast(const struct sockaddr_in *addr) {
    return static_cast<const SA *>(reinterpret_cast<const void *>(addr));
}

const SA *sockaddr_cast(const struct sockaddr_in6 *addr) {
    return static_cast<const SA *>(reinterpret_cast<const void *>(addr));
}

SA *sockaddr_cast(struct sockaddr_in *addr) {
    return static_cast<SA *>(reinterpret_cast<void *>(addr));
}

SA *sockaddr_cast(struct sockaddr_in6 *addr) {
    return static_cast<SA *>(reinterpret_cast<void *>(addr));
}

const struct sockaddr_in *sockaddr_in_cast(const struct sockaddr *addr) {
    return static_cast<const struct sockaddr_in *>(reinterpret_cast<const void *>(addr));
}

const struct sockaddr_in6 *sockaddr_in6_cast(const struct sockaddr *addr) {
    return static_cast<const struct sockaddr_in6 *>(reinterpret_cast<const void *>(addr));
}

int createNonblockingOrDie(sa_family_t family) {
#if VALGRIND
    int sockfd = ::socket(family, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0) {
        LOG_SYSFATAL << "sockets::createNonblockingOrDie";
    }
    setNonBlockAndCloseOnExec(sockfd);
#else
    int sockfd = ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0) {
        LOG_SYSFATAL << "sockets::createNonblockingOrDie";
    }
#endif
    return sockfd;
}

void bindOrDie(int sockfd, const struct sockaddr *addr) {
    int ret = ::bind(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
    if (ret < 0) {
        LOG_SYSFATAL << "sockets::bindOrDie";
    }
}

void listenOrDie(int sockfd) {
    int ret = ::listen(sockfd, SOMAXCONN);
    if (ret < 0) {
        LOG_SYSFATAL << "sockets::listenOrDie";
    }
}

int accept(int sockfd, struct sockaddr_in6 *addr) {
    socklen_t addrlen = static_cast<socklen_t>(sizeof *addr);
#if VALGRIND || defined(NO_ACCEPT4)
    int connfd = ::accept(sockfd, sockaddr_cast(addr), &addrlen);
    setNonBlockAndCloseOnExec(connfd);
#else
    int connfd = ::accept4(sockfd, sockaddr_cast(addr), &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
#endif
    if (connfd < 0) {
        int savedErrno = errno;
        LOG_SYSERR << "Socket::accept";
        switch (savedErrno) {
            case EAGAIN:
            case ECONNABORTED:
            case EINTR:
            case EPROTO:  // ???
            case EPERM:
            case EMFILE:  // per-process lmit of open file desctiptor ???
                // expected errors
                errno = savedErrno;
                break;
            case EBADF:
            case EFAULT:
            case EINVAL:
            case ENFILE:
            case ENOBUFS:
            case ENOMEM:
            case ENOTSOCK:
            case EOPNOTSUPP:
                // unexpected errors
                LOG_FATAL << "unexpected error of ::accept " << savedErrno;
                break;
            default:
                LOG_FATAL << "unknown error of ::accept " << savedErrno;
                break;
        }
    }
    return connfd;
}

int connect(int sockfd, const struct sockaddr *addr) {
    return ::connect(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
}

ssize_t read(int sockfd, void *buf, size_t count) { return ::read(sockfd, buf, count); }

ssize_t readv(int sockfd, const struct iovec *iov, int iovcnt) {
    return ::readv(sockfd, iov, iovcnt);
}

ssize_t write(int sockfd, const void *buf, size_t count) { return ::write(sockfd, buf, count); }

void close(int sockfd) {
    if (::close(sockfd) < 0) {
        LOG_SYSERR << "sockets::close";
    }
}

void shutdownWrite(int sockfd) {
    if (::shutdown(sockfd, SHUT_WR) < 0) {
        LOG_SYSERR << "sockets::shutdownWrite";
    }
}

void toIpPort(char *buf, size_t size, const struct sockaddr *addr) {
    if (addr->sa_family == AF_INET6) {
        buf[0] = '[';
        toIp(buf + 1, size - 1, addr);
        size_t end = ::strlen(buf);
        const struct sockaddr_in6 *addr6 = sockaddr_in6_cast(addr);
        uint16_t port = sockets::networkToHost16(addr6->sin6_port);
        assert(size > end);
        snprintf(buf + end, size - end, "]:%u", port);
        return;
    }
    toIp(buf, size, addr);
    size_t end = ::strlen(buf);
    const struct sockaddr_in *addr4 = sockaddr_in_cast(addr);
    uint16_t port = sockets::networkToHost16(addr4->sin_port);
    assert(size > end);
    snprintf(buf + end, size - end, ":%u", port);
}

void toIp(char *buf, size_t size, const struct sockaddr *addr) {
    if (addr->sa_family == AF_INET) {
        assert(size >= INET_ADDRSTRLEN);
        const struct sockaddr_in *addr4 = sockaddr_in_cast(addr);
        ::inet_ntop(AF_INET, &addr4->sin_addr, buf, static_cast<socklen_t>(size));
    } else if (addr->sa_family == AF_INET6) {
        assert(size >= INET6_ADDRSTRLEN);
        const struct sockaddr_in6 *addr6 = sockaddr_in6_cast(addr);
        ::inet_ntop(AF_INET6, &addr6->sin6_addr, buf, static_cast<socklen_t>(size));
    }
}

void fromIpPort(const char *ip, uint16_t port, struct sockaddr_in *addr) {
    addr->sin_family = AF_INET;
    addr->sin_port = hostToNetwork16(port);
    if (::inet_pton(AF_INET, ip, &addr->sin_addr) <= 0) {
        LOG_SYSERR << "sockets::fromIpPort";
    }
}

void fromIpPort(const char *ip, uint16_t port, struct sockaddr_in6 *addr) {
    addr->sin6_family = AF_INET6;
    addr->sin6_port = hostToNetwork16(port);
    if (::inet_pton(AF_INET6, ip, &addr->sin6_addr) <= 0) {
        LOG_SYSERR << "sockets::fromIpPort";
    }
}

int getSocketError(int sockfd) {
    int optval;
    socklen_t optlen = static_cast<socklen_t>(sizeof optval);

    if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
        return errno;
    } else {
        return optval;
    }
}

struct sockaddr_in6 getLocalAddr(int sockfd) {
    struct sockaddr_in6 localaddr;
    memZero(&localaddr, sizeof localaddr);
    socklen_t addrlen = static_cast<socklen_t>(sizeof localaddr);
    if (::getsockname(sockfd, sockaddr_cast(&localaddr), &addrlen) < 0) {
        LOG_SYSERR << "sockets::getLocalAddr";
    }
    return localaddr;
}

struct sockaddr_in6 getPeerAddr(int sockfd) {
    struct sockaddr_in6 peeraddr;
    memZero(&peeraddr, sizeof peeraddr);
    socklen_t addrlen = static_cast<socklen_t>(sizeof peeraddr);
    if (::getpeername(sockfd, sockaddr_cast(&peeraddr), &addrlen) < 0) {
        LOG_SYSERR << "sockets::getPeerAddr";
    }
    return peeraddr;
}

bool isSelfConnect(int sockfd) {
    struct sockaddr_in6 localaddr = getLocalAddr(sockfd);
    struct sockaddr_in6 peeraddr = getPeerAddr(sockfd);
    if (localaddr.sin6_family == AF_INET) {
        const struct sockaddr_in *laddr4 = reinterpret_cast<struct sockaddr_in *>(&localaddr);
        const struct sockaddr_in *raddr4 = reinterpret_cast<struct sockaddr_in *>(&peeraddr);
        return laddr4->sin_port == raddr4->sin_port &&
               laddr4->sin_addr.s_addr == raddr4->sin_addr.s_addr;
    } else if (localaddr.sin6_family == AF_INET6) {
        return localaddr.sin6_port == peeraddr.sin6_port &&
               memcmp(&localaddr.sin6_addr, &peeraddr.sin6_addr, sizeof localaddr.sin6_addr) == 0;
    } else {
        return false;
    }
}

}  // namespace sockets
}  // namespace dws::net
