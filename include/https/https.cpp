#include "pch.hpp"
#include "https.hpp"

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

void https::listener(_server_data server_data)
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

    SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);
    SSL_CTX_set_cipher_list(ctx, "ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305");

    SOCKET socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (char*)&enable, sizeof(enable));
#ifdef TCP_FASTOPEN
    setsockopt(socket, IPPROTO_TCP, TCP_FASTOPEN, (char*)&enable, sizeof(enable));
#endif
#ifdef TCP_DEFER_ACCEPT // @note unix
    setsockopt(socket, IPPROTO_TCP, TCP_DEFER_ACCEPT, &enable, sizeof(enable));
#endif
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
            "Connection: close\r\n\r\n"
            "{}",
            Content.size(), Content);

    listen(socket, SOMAXCONN); // @todo
    while (true)
    {
        SOCKET fd = accept(socket, (struct sockaddr*)&addr, &addrlen);
        if (fd < 0) continue;

        setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*)&enable, sizeof(enable));

        SSL *ssl = SSL_new(ctx);
        SSL_set_fd(ssl, fd);
        
        if (SSL_accept(ssl) > 0)
        {

            char buf[213]; // @note size of growtopia's POST request.
            const int length{ sizeof(buf) };

            if (SSL_read(ssl, buf, length) == length)
            {
                printf("%s", buf);
                
                if (std::string_view(buf, sizeof(buf )).contains("POST /growtopia/server_data.php HTTP/1.1"))
                {
                    SSL_write(ssl, response.c_str(), response.size());
                    SSL_shutdown(ssl);
                }
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