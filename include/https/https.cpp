#include "pch.hpp"
#include "https.hpp"

#include "ssl/openssl/err.h"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <sys/socket.h>

    #define SOCKET int
#endif

#if defined(_MSC_VER)
    using namespace std::chrono;
#else
    using namespace std::chrono::_V2;
#endif
using namespace std::literals::chrono_literals;

void https::listener(_server_data server_data)
{
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());
    if (ctx == nullptr)
        ERR_print_errors_fp(stderr);

    if (SSL_CTX_use_certificate_file(ctx, "resources/ctx/server.crt", SSL_FILETYPE_PEM) <= 0 ||
        SSL_CTX_use_PrivateKey_file(ctx, "resources/ctx/server.key", SSL_FILETYPE_PEM) <= 0)
            ERR_print_errors_fp(stderr);

    SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION);
    
    SSL_CTX_set_cipher_list(ctx, "HIGH:!aNULL:!MD5:!RC4:!3DES");
    SSL_CTX_set_ecdh_auto(ctx, 1);

    SOCKET socket = ::socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(443);
    socklen_t addrlen = sizeof(addr);
    if (bind(socket, (struct sockaddr*)&addr, addrlen) < 0)
        puts("could not bind port 443.");

    printf("listening on %s:%d\n", server_data.server.c_str(), server_data.port);

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
            server_data.server, server_data.port, server_data.type, server_data.type2, server_data.maint, server_data.loginurl, server_data.meta
        );
    const std::string response =
        std::format(
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: {}\r\n"
            "Connection: close\r\n"
            "\r\n{}",
            Content.size(), Content).c_str();

#ifndef _WIN32
    setsockopt(socket, IPPROTO_TCP, TCP_DEFER_ACCEPT, &enable, sizeof(enable));
#endif

    listen(socket, 9);
    while (true)
    {
        SOCKET fd = accept(socket, (struct sockaddr*)&addr, &addrlen);
        if (fd < 0) continue;

        SSL *ssl = SSL_new(ctx);
        SSL_set_fd(ssl, fd);

        struct timeval timeout{.tv_sec = 3};
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));

        if (SSL_accept(ssl) > 0)
        {
            char buf[213]; // @note size of growtopia's POST request.
            int length{ sizeof(buf ) };

            if (SSL_read(ssl, buf, length) == length)
            {
                printf("%s", buf);
                if (std::string_view{buf, sizeof(buf )}.contains("POST /growtopia/server_data.php HTTP/1.1"))
                    SSL_write(ssl, response.c_str(), response.size());
            }
            else ERR_print_errors_fp(stderr); // @note we don't accept growtopia GET. this error is normal if appears.
        }
        else ERR_print_errors_fp(stderr);
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