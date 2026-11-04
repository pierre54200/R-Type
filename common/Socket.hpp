#pragma once

#include <stdexcept>

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>

    #ifdef _MSC_VER
        #pragma comment(lib, "ws2_32.lib")
    #endif

    using socket_t      = SOCKET;
    using socket_len_t  = int;
    constexpr socket_t  INVALID_SOCK = INVALID_SOCKET;

    inline void closeSocket(socket_t s) { closesocket(s); }

    inline bool setNonBlocking(socket_t s)
    {
        u_long mode = 1;
        return ioctlsocket(s, FIONBIO, &mode) == 0;
    }

    inline void ensureNetworkInit()
    {
        struct Guard
        {
            Guard()
            {
                WSADATA data;
                if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
                    throw std::runtime_error("WSAStartup failed");
            }
            ~Guard() { WSACleanup(); }
        };
        static Guard guard;
    }
#else
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <fcntl.h>
    #include <sys/select.h>
    #include <unistd.h>

    using socket_t      = int;
    using socket_len_t  = socklen_t;
    constexpr socket_t  INVALID_SOCK = -1;

    inline void closeSocket(socket_t s) { close(s); }

    inline bool setNonBlocking(socket_t s)
    {
        int flags = fcntl(s, F_GETFL, 0);
        return flags >= 0 && fcntl(s, F_SETFL, flags | O_NONBLOCK) == 0;
    }

    inline void ensureNetworkInit() {}
#endif

// Attend qu'une socket soit lisible, au plus timeoutMs millisecondes.
// select() existe sous Linux comme sous Windows, contrairement a poll().
inline bool waitReadable(socket_t s, int timeoutMs)
{
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(s, &readSet);
    timeval timeout{};
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;
    return select(static_cast<int>(s) + 1, &readSet, nullptr, nullptr, &timeout) > 0;
}
