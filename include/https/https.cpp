#include "pch.hpp"
#include "https.hpp"
#include "server_data.hpp"

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
#endif

void https::listener()
{
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    constexpr int enable = 1;

    SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());
    if (ctx == nullptr)
        ERR_print_errors_fp(stderr);

    if (SSL_CTX_use_certificate_file(ctx, "resources/ctx/server.crt", SSL_FILETYPE_PEM) <= 0 ||
        SSL_CTX_use_PrivateKey_file(ctx, "resources/ctx/server.key", SSL_FILETYPE_PEM) <= 0)
            ERR_print_errors_fp(stderr);

    SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);

    SOCKET socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

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
    if (bind(socket, (struct sockaddr*)&addr, addrlen) < 0)
        puts("could not bind port 443.");

    std::printf("listening on %s:%hu\n", g_server_data.server.c_str(), g_server_data.port);

    const std::string Content =
        std::format(
            "server|{}\n"
            "port|{}\n"
            "type|{}\n"
            "type2|{}\n"
            "#maint|{}\n"
            "loginurl|{}\n"
            "meta|{}\n"
            "RTENDMARKERBS1001", 
            g_server_data.server, g_server_data.port, g_server_data.type, g_server_data.type2, g_server_data.maint, g_server_data.loginurl, g_server_data.meta
        );
    const std::string response =
        std::format(
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: {}\r\n"
            "Connection: close\r\n\r\n"
            "{}",
            Content.size(), Content);

    listen(socket, SOMAXCONN); // @todo
    while (true)
    {
        SOCKET fd = accept(socket, reinterpret_cast<sockaddr*>(&addr), &addrlen);
        if (fd < 0) continue;

        setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&enable, sizeof(enable));

        SSL *ssl = SSL_new(ctx);
        if (!ssl) { 
#ifdef _WIN32
            closesocket(fd);
#else
            close(fd);
#endif
            continue;
        }
        if (SSL_set_fd(ssl, fd) != 1) {
#ifdef _WIN32
            closesocket(fd);
#else
            close(fd);
#endif
            continue;
        }
        if (SSL_accept(ssl) > 0)
        {

            char buf[213]; // @note size of growtopia's POST request.
            const int length{ sizeof(buf) };

            if (SSL_read(ssl, buf, length) == length)
            {
                puts(buf);
                
                if (std::string_view(buf, sizeof(buf )).contains("POST /growtopia/server_data.php HTTP/1.1"))
                {
                    SSL_write(ssl, response.c_str(), response.size());
                    SSL_shutdown(ssl);
                }
            }
            else ERR_print_errors_fp(stderr); // @note we don't accept growtopia GET. this error is normal if appears.
        }
        else ERR_print_errors_fp(stderr);

        SSL_shutdown(ssl);
        SSL_free(ssl);
#ifdef _WIN32
            closesocket(fd);
#else
            close(fd);
#endif
    }
}

#ifndef _WIN32
    #undef SOCKET
#endif