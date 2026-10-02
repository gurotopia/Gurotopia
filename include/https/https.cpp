#include "pch.hpp"
#include "https.hpp"
#include "server_data.hpp"
#include <fstream>

/* which address a player gets: on this PC 127.0.0.1, on the home network (or a VPN) the address they
   reached us on, over the internet the public address in public_ip.txt */
static bool is_private(const u_char *b)
{
    return b[0] == 10 || b[0] == 127 || (b[0] == 172 && b[1] >= 16 && b[1] <= 31) || (b[0] == 192 && b[1] == 168) ||
           (b[0] == 169 && b[1] == 254) || (b[0] == 100 && b[1] >= 64 && b[1] <= 127);
}
static std::string ip_text(const u_char *b) { return std::format("{}.{}.{}.{}", b[0], b[1], b[2], b[3]); }
static std::string public_ip()
{
    std::ifstream f("public_ip.txt");
    std::string ip{};
    std::getline(f, ip);
    while (!ip.empty() && std::isspace(static_cast<unsigned char>(ip.back()))) ip.pop_back();
    return ip;
}
static std::string make_response(const std::string &server_ip)
{
    const std::string content = std::format(
        "server|{}\nport|{}\ntype|{}\ntype2|{}\n#maint|{}\nloginurl|{}\nmeta|{}\nRTENDMARKERBS1001",
        server_ip, gServer_data.port, gServer_data.type, gServer_data.type2, gServer_data.maint, gServer_data.loginurl, gServer_data.meta);
    return std::format("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: {}\r\nConnection: close\r\n\r\n{}", content.size(), content);
}

#include <openssl/err.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h> // @note TCP_DEFER_ACCEPT
    #include <sys/socket.h>

    #define SOCKET int
    #define INVALID_SOCKET	(SOCKET)(~0)
    #define SOCKET_ERROR	(-1)

#endif

/* cross-platform socket close */
static void cross_close(SOCKET fd)
{
#ifdef _WIN32
    closesocket(fd);
#else // @note unix
    close(fd);
#endif
}

/* cross-platform error log */
static void cross_log(const std::string &message)
{
#ifdef _WIN32
    std::fprintf(stderr, "%s: %d\n", message.c_str(), WSAGetLastError());
#else // @note unix
    std::fprintf(stderr, "%s: %s\n", message.c_str(), strerror(errno));
#endif
}

void https::listener()
{
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    constexpr int enable = 1;

    /* https://docs.openssl.org/3.0/man3/SSL_CTX_new/#return-values */
    SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx)
    {
        ERR_print_errors_fp(stderr);
    }

    /* https://docs.openssl.org/master/man3/SSL_CTX_use_certificate/#return-values */
    if (SSL_CTX_use_certificate_file(ctx, "resources/ctx/server.crt", SSL_FILETYPE_PEM) != 1 ||
        SSL_CTX_use_PrivateKey_file(ctx, "resources/ctx/server.key", SSL_FILETYPE_PEM)  != 1)
    {
        ERR_print_errors_fp(stderr);
    }

    SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);

#ifdef SIGPIPE // @note unix
    std::signal(SIGPIPE, SIG_IGN);
#endif

    /* https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-socket */
    SOCKET socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket == INVALID_SOCKET)
    {
        cross_log("socket function failed");
    }

#ifdef SO_REUSEADDR
    setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (char*)&enable, sizeof(enable));
#endif
#ifdef TCP_DEFER_ACCEPT // @note unix
    setsockopt(socket, IPPROTO_TCP, TCP_DEFER_ACCEPT, (char*)&enable, sizeof(enable));
#endif
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(443);
    socklen_t addrlen = sizeof(addr);

    /* https://learn.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-bind */
    if (bind(socket, (struct sockaddr*)&addr, addrlen) == SOCKET_ERROR)
    {
        cross_log("could not bind socket");
    }

    const std::string Content =
        std::format(
            "server|{}\n"
            "port|{}\n"
            "type|{}\n"
            "type2|{}\n" // @todo remove for older clients
            "#maint|{}\n"
            "loginurl|{}\n"
            "meta|{}\n"
            "RTENDMARKERBS1001", 
            gServer_data.server, gServer_data.port, gServer_data.type, gServer_data.type2, gServer_data.maint, gServer_data.loginurl, gServer_data.meta
        );
    const std::string response =
        std::format(
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: {}\r\n"
            "Connection: close\r\n\r\n"
            "{}",
            Content.size(), Content);

    /* https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-listen */
    if (listen(socket, SOMAXCONN) == SOCKET_ERROR)
    {
        cross_log("failed to listen on socket");
    }
    else std::printf("listening on %s:%hu\n", gServer_data.server.c_str(), gServer_data.port);
    
    while (true)
    {
        /* https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-accept */
        SOCKET fd = accept(socket, reinterpret_cast<sockaddr*>(&addr), &addrlen);
        if (fd == INVALID_SOCKET) continue;

        const u_char *cb = reinterpret_cast<const u_char*>(&addr.sin_addr);
        const std::string client_ip = ip_text(cb);
        std::string server_ip = gServer_data.server;
        {
            sockaddr_in local{};
            decltype(addrlen) ll = sizeof(local);
            if (getsockname(fd, reinterpret_cast<sockaddr*>(&local), &ll) == 0)
                server_ip = ip_text(reinterpret_cast<const u_char*>(&local.sin_addr)); // @note the address they reached us on
            if (!is_private(cb)) { const std::string pub = public_ip(); if (!pub.empty()) server_ip = pub; }
        }
        
        SSL *ssl = SSL_new(ctx);
        if (!ssl) {
            cross_close(fd);
            continue;
        }
        if (SSL_set_fd(ssl, fd) != 1) {
            cross_close(fd);
            continue;
        }
        if (SSL_accept(ssl) > 0)
        {
            char buf[4096]; // @note any size of request
            std::string request{}; // @note read the whole request (headers + body) before answering
            for (int tries = 0; tries < 16 && request.size() < 16384; ++tries)
            {
                const int n = SSL_read(ssl, buf, sizeof(buf));
                if (n <= 0) break;
                request.append(buf, static_cast<std::size_t>(n));
                const std::size_t end = request.find("\r\n\r\n");
                if (end == std::string::npos) continue;
                std::string head = request.substr(0, end);
                for (char &ch : head) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
                std::size_t need = end + 4;
                if (const std::size_t cl = head.find("content-length:"); cl != std::string::npos)
                    need += static_cast<std::size_t>(std::atoi(head.c_str() + cl + 15));
                if (request.size() >= need) break;
            }
            const int length = static_cast<int>(request.size());

            if (length > 0)
            {
                // @note full request printing removed
                std::string content = request;
                
                if (content.find("POST /growtopia/server_data.php HTTP/1.1") != std::string_view::npos)
                {
                    { const std::string r = make_response(server_ip); SSL_write(ssl, r.c_str(), static_cast<int>(r.size())); remember_address(client_ip, server_ip); std::printf("[login] %s -> %s\n", client_ip.c_str(), server_ip.c_str()); }
                }
            }
            else ERR_print_errors_fp(stderr); // @note we don't accept growtopia GET. this error is normal if appears.
        }
        else ERR_print_errors_fp(stderr);

        SSL_shutdown(ssl);
        SSL_free(ssl);
        cross_close(fd);
    }
}

#ifndef _WIN32
    #undef SOCKET
#endif
