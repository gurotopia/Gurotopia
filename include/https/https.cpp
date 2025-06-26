#include "pch.hpp"
#include "https.hpp"

#include "ssl/openssl/err.h"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <io.h>
#else
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <sys/socket.h>

    #define SOCKET int
#endif

void https::listener()
{
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    const SSL_METHOD *method = TLS_server_method();

    SSL_CTX *ctx = SSL_CTX_new(method);
    if (!ctx)
        ERR_print_errors_fp(stderr);

    if (SSL_CTX_use_certificate_file(ctx, "ssl/server.crt", SSL_FILETYPE_PEM) <= 0)
        ERR_print_errors_fp(stderr);

    if (SSL_CTX_use_PrivateKey_file(ctx, "ssl/server.key", SSL_FILETYPE_PEM) <= 0)
        ERR_print_errors_fp(stderr);

    SOCKET socket = ::socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(443);
    bind(socket, (struct sockaddr*)&addr, sizeof(addr));

    listen(socket, 10);
    while (true)
    {
        socklen_t addrlen = sizeof(addr);
        SOCKET fd = accept(socket, (struct sockaddr*)&addr, &addrlen);
        
        SSL *ssl = SSL_new(ctx);
        SSL_set_fd(ssl, fd);
        if (SSL_accept(ssl) <= 0) 
        {
            ERR_print_errors_fp(stderr);
#ifdef _WIN32
            closesocket(fd);
#else
            close(fd);
#endif
            SSL_free(ssl);
            continue;
        }
    }
}

#ifndef _WIN32
    #undef SOCKET
#endif