#pragma once

#if defined _WIN32
#include <ws2tcpip.h>
#include <iphlpapi.h>
//#include <afunix.h>
#define bzero(ptr,n) memset(ptr, 0, n)
#else
#include <sys/socket.h> // basic socket definitions
#include <netinet/in.h> // sockaddr_in{} and other Internet defns
#include <arpa/inet.h>  // inet(3) functions
#include <sys/ioctl.h>
#include <netdb.h>
#include <net/if.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#endif

#include <iostream>
#include <sstream>
#include <stdexcept>

template <typename... Args>
void Throw(Args&&... args)
{
    std::stringstream err;

    using expander = int[];
    expander
    {
        (void(err << std::forward<Args>(args)), 0)...
    };

    std::cerr << err.str() << std::endl;

    std::terminate();
}

//#define THROW(Message)                      \
//    std::stringstream err;                  \
//    err << Message;                         \
//    throw std::runtime_error(err.str());    \

constexpr auto SERV_ADDR = "224.0.0.1";
constexpr auto SERV_PORT = "9877";

namespace UDP
{

constexpr auto MAX_LINE = 4096;

using Buffer = char[MAX_LINE];

class Socket final
{
public:
    Socket()
    {
#if defined _WIN32
        // Initialize Winsock
        WSADATA wsaData;

        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0)
        {
            Throw("Initialize Winsock failed: ", result);
        }
#endif
    }

    ~Socket()
    {
        close_socket(sockfd);

#if defined _WIN32
        WSACleanup();
#endif
    }

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    Socket(const Socket&&) = delete;
    Socket& operator=(const Socket&&) = delete;

    void bind(const char *host, const char *serv)
    {
        if (udp_mcast_server(host, serv, nullptr) < 0)
        {
            Throw("udp bind error for ", host, ":", serv);
        }
    }

    void connect(const char *host, const char *serv)
    {
        if (udp_mcast_client(host, serv) < 0)
        {
            Throw("udp connect error for ", host, ":", serv);
        }
    }

    int recv_from(Buffer& buffer, std::string& endpoint)
    {
        int n;
        socklen_t len = sizeof(saddr);

        if ((n = recvfrom(sockfd, buffer, MAX_LINE - 1, 0, (sockaddr *)&saddr, &len)) < 0)
        {
            Throw("recvfrom error");
        }

        buffer[n] = '\0'; // null terminate

        if ((endpoint = sock_ntop((sockaddr *)&saddr, len)).empty())
        {
            Throw("sock_ntop error");
        }

        return n;
    }

    void send_to(const char *buffer, size_t length)
    {
        socklen_t len = sizeof(saddr);

        if (sendto(sockfd, buffer, length, 0, (sockaddr *)&saddr, len) != length)
        {
            Throw("sendto error");
        }
    }

    int recv(Buffer& buffer)
    {
        int n;

        if ((n = ::recv(sockfd, buffer, MAX_LINE - 1, 0)) < 0)
        {
            Throw("recv error");
        }

        buffer[n] = '\0'; // null terminate

        return n;
    }

    void send(const char *buffer, size_t length)
    {
        if (::send(sockfd, buffer, length, 0) != length)
        {
            Throw("send error");
        }
    }

private:
    int udp_server(const char *host, const char *serv, socklen_t *addrlenp)
    {
        int n;
        addrinfo hints, *res, *ressave;

        bzero(&hints, sizeof(addrinfo));
        hints.ai_flags = AI_PASSIVE;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;

        if ((n = getaddrinfo(host, serv, &hints, &res)) != 0)
        {
            Throw("getaddrinfo error for ", host, ":", serv);
        }

        ressave = res;

        do {
            sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
            if (sockfd < 0)
                continue; // error - try next one

            if (::bind(sockfd, res->ai_addr, res->ai_addrlen) == 0)
                break; // success

            close_socket(sockfd); // bind error - close and try next one
        } while ((res = res->ai_next) != nullptr);

        if (res == nullptr) // errno from final socket() or bind()
        {
            Throw("errno from final socket() or bind() for ", host, ":", serv);
        }

        if (addrlenp)
            *addrlenp = res->ai_addrlen; // return size of protocol address

        freeaddrinfo(ressave);

        return sockfd;
    }

    int udp_connect(const char *host, const char *serv)
    {
        int n;
        addrinfo hints, *res, *ressave;

        bzero(&hints, sizeof(addrinfo));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;

        if ((n = getaddrinfo(host, serv, &hints, &res)) != 0)
        {
            Throw("getaddrinfo error for ", host, ":", serv);
        }

        ressave = res;

        do {
            sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
            if (sockfd < 0)
                continue; // ignore this one

            if (::connect(sockfd, res->ai_addr, res->ai_addrlen) == 0)
                break; // success

            close_socket(sockfd); // ignore this one
        } while ((res = res->ai_next) != nullptr);

        if (res == nullptr) // errno set from final connect()
        {
            Throw("errno set from final connect() for ", host, ":", serv);
        }

        freeaddrinfo(ressave);

        return sockfd;
    }

    int udp_mcast_server(const char *host, const char *serv, socklen_t *addrlenp)
    {
        sockaddr_in servaddr, grpaddr;

        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port = htons(atoi(serv));

        if ((sockfd = socket(servaddr.sin_family, SOCK_DGRAM, 0)) < 0)
        {
            Throw("socket error");
        }

        if (::bind(sockfd, (sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        {
            Throw("bind error");
        }

        bzero(&grpaddr, sizeof(grpaddr));
        grpaddr.sin_family = AF_INET;

        int n;
        if ((n = inet_pton(grpaddr.sin_family, host, &grpaddr.sin_addr.s_addr)) <= 0)
        {
            Throw("inet_pton error for %s", host);
        }

        if (mcast_join(sockfd, (sockaddr *)&grpaddr, sizeof(grpaddr), nullptr, 0) < 0)
        {
            Throw("mcast_join error");
        }

        return sockfd;
    }

    int udp_mcast_client(const char *host, const char *serv)
    {
        char ttl = 1;
        sockaddr_in cliAddr;
        hostent *h;

        h = gethostbyname(host);
        if (h == nullptr)
        {
            Throw("client: unknown host '", host, "'");
        }

        saddr.sin_family = h->h_addrtype;
        memcpy((char *)&saddr.sin_addr.s_addr, h->h_addr_list[0], h->h_length);
        saddr.sin_port = htons(atoi(serv));

        // check if dest address is multicast
        if (!IN_MULTICAST(ntohl(saddr.sin_addr.s_addr)))
        {
            Throw("client: given address '", inet_ntoa(saddr.sin_addr), "' is not multicast");
        }

        // create socket
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0)
        {
            Throw("client: cannot open socket");
        }

        // bind any port number
        cliAddr.sin_family = AF_INET;
        cliAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        cliAddr.sin_port = htons(0);
        if (::bind(sockfd, (sockaddr *)&cliAddr, sizeof(cliAddr)) < 0)
        {
            Throw("bind error");
        }

        if (setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0)
        {
            Throw("client: cannot set ttl = ", ttl);
        }

        printf("client: sending data on multicast group '%s' (%s:%s)\n",
            h->h_name, inet_ntoa(*(in_addr *)h->h_addr_list[0]), serv);

        return sockfd;
    }

    int mcast_join(int sockfd, const sockaddr *grp, socklen_t grplen,
            const char *ifname, u_int ifindex)
    {
#if defined(MCAST_JOIN_GROUP) && defined(_WIN32)
        group_req req;
        if (ifindex > 0) {
            req.gr_interface = ifindex;
        }
        else if (ifname != nullptr) {
            if ((req.gr_interface = if_nametoindex(ifname)) == 0) {
                errno = ENXIO; // i/f name not found
                return -1;
            }
        }
        else {
            req.gr_interface = 0;
        }

        if (grplen > sizeof(req.gr_group)) {
            errno = EINVAL;
            return -1;
        }

        memcpy(&req.gr_group, grp, grplen);

        int level;

        if ((level = family_to_level(grp->sa_family)) < 0)
        {
            Throw("family_to_level error");
        }

        return setsockopt(sockfd, level, MCAST_JOIN_GROUP, (char *)&req, sizeof(req));
#else
        switch (grp->sa_family) {
        case AF_INET: {
            ip_mreq mreq;
            ifreq ifreq;

            memcpy(&mreq.imr_multiaddr,
                &((const sockaddr_in *) grp)->sin_addr,
                sizeof(in_addr));

            if (ifindex > 0) {
                if (if_indextoname(ifindex, ifreq.ifr_name) == nullptr) {
                    errno = ENXIO; //* i/f index not found
                    return -1;
                }
                goto doioctl;
            }
            else if (ifname != nullptr) {
                strncpy(ifreq.ifr_name, ifname, IFNAMSIZ);
            doioctl:
                if (ioctl(sockfd, SIOCGIFADDR, &ifreq) < 0)
                    return -1;
                memcpy(&mreq.imr_interface,
                    &((sockaddr_in *) &ifreq.ifr_addr)->sin_addr,
                    sizeof(in_addr));
            }
            else
                mreq.imr_interface.s_addr = htonl(INADDR_ANY);

            return setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
        }
        default:
            errno = EAFNOSUPPORT;
            return -1;
        }
#endif
    }

    int family_to_level(int family)
    {
        switch (family) {
        case AF_INET:
            return IPPROTO_IP;
        default:
            return -1;
        }
    }

    char *sock_ntop(const sockaddr *sa, socklen_t salen)
    {
        char portstr[8];
        static char str[128];

        switch (sa->sa_family)
        {
        case AF_INET: {
            sockaddr_in *sin = (sockaddr_in *)sa;

            if (inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str)) == nullptr)
            {
                return nullptr;
            }

            if (ntohs(sin->sin_port) != 0)
            {
                snprintf(portstr, sizeof(portstr), ":%d", ntohs(sin->sin_port));
                strcat(str, portstr);
            }

            return str;
        }

/*#ifdef AF_UNIX
        case AF_UNIX: {
            sockaddr_un *unp = (sockaddr_un *)sa;

            // OK to have no pathname bound to the socket: happens on
            // every connect() unless client calls bind() first.
            if (unp->sun_path[0] == 0)
                strcpy(str, "(no pathname bound)");
            else
                snprintf(str, sizeof(str), "%s", unp->sun_path);
            return str;
        }
#endif*/

#ifdef HAVE_SOCKADDR_DL_STRUCT
        case AF_LINK: {
            sockaddr_dl *sdl = (sockaddr_dl *)sa;

            if (sdl->sdl_nlen > 0)
                snprintf(str, sizeof(str), "%*s (index %d)",
                    sdl->sdl_nlen, &sdl->sdl_data[0], sdl->sdl_index);
            else
                snprintf(str, sizeof(str), "AF_LINK, index=%d", sdl->sdl_index);
            return str;
        }
#endif

        default:
            snprintf(str, sizeof(str), "sock_ntop: unknown AF_xxx: %d, len %d", sa->sa_family, salen);
            return str;
        }

        return nullptr;
    }

    void close_socket(int fd)
    {
        if (shutdown(fd, 2) != 0) //win: SD_BOTH, unix: SHUT_RDWR
        {
            Throw("socket shutdown error");
        }
    }

private:
    int sockfd;
    sockaddr_in saddr;
};

} // namespace UDP
