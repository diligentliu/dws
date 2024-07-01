#include "Socket.h"

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <cstdio>  // snprintf

#include "InetAddress.h"
#include "Logging.h"
#include "SocketsOps.h"

namespace dws::net {

Socket::~Socket() { sockets::close(sockfd_); }

bool Socket::getTcpInfo(struct tcp_info *tcpi) const {
    socklen_t len = sizeof(*tcpi);
    std::memset(tcpi, 0, len);
    return ::getsockopt(sockfd_, SOL_TCP, TCP_INFO, tcpi, &len) == 0;
}

bool Socket::getTcpInfoString(char *buf, int len) const {
    struct tcp_info tcpi {};
    bool res = getTcpInfo(&tcpi);
    if (res) {
        snprintf(buf, len,
                 "unrecovered=%u "
                 "rto=%u ato=%u snd_mss=%u rcv_mss=%u "
                 "lost=%u retrans=%u rtt=%u rttvar=%u "
                 "sshthresh=%u cwnd=%u total_retrans=%u",
                 tcpi.tcpi_retransmits, tcpi.tcpi_rto, tcpi.tcpi_ato, tcpi.tcpi_snd_mss,
                 tcpi.tcpi_rcv_mss, tcpi.tcpi_lost, tcpi.tcpi_retrans, tcpi.tcpi_rtt,
                 tcpi.tcpi_rttvar, tcpi.tcpi_snd_ssthresh, tcpi.tcpi_snd_cwnd,
                 tcpi.tcpi_total_retrans);
    }
    return res;
}

void Socket::bindAddress(const dws::net::InetAddress &localaddr) {
    sockets::bindOrDie(sockfd_, localaddr.getSockAddr());
}

void Socket::listen() { sockets::listenOrDie(sockfd_); }

int Socket::accept(dws::net::InetAddress *peeraddr) {
    struct sockaddr_in6 addr {};
    bzero(&addr, sizeof addr);
    int connfd = sockets::accept(sockfd_, &addr);
    if (connfd >= 0) {
        peeraddr->setSockAddrInet6(addr);
    }
    return connfd;
}

void Socket::shutdownWrite() { sockets::shutdownWrite(sockfd_); }

void Socket::setTcpNoDelay(bool on) {
    int optval = static_cast<int>(on);
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, static_cast<socklen_t>(sizeof optval));
}

void Socket::setReuseAddr(bool on) {
    int optval = static_cast<int>(on);
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, static_cast<socklen_t>(sizeof optval));
}

void Socket::setReusePort(bool on) {
#ifdef SO_REUSEPORT
    int optval = static_cast<int>(on);
    int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval,
                           static_cast<socklen_t>(sizeof optval));
    if (ret < 0 && on) {
        LOG_SYSERR << "[Socket::setReusePort] SO_REUSEPORT failed";
    }
#else
    if (on) {
        LOG(ERROR) << "[Socket::setReusePort] SO_REUSEPORT isn't supported";
    }
#endif
}

void Socket::setKeepAlive(bool on) {
    int optval = static_cast<int>(on);
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, static_cast<socklen_t>(sizeof optval));
}

}  // namespace dws::net
