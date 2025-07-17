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

std::unordered_map<std::string, https::client> https::clients{};

void https::listener(std::string ip, short enet_port)
{
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());
    if (ctx == nullptr)
        ERR_print_errors_fp(stderr);

    if (SSL_CTX_use_certificate_file(ctx, "resources/ctx/server.crt", SSL_FILETYPE_PEM) <= 0 ||
        SSL_CTX_use_PrivateKey_file(ctx, "resources/ctx/server.key", SSL_FILETYPE_PEM) <= 0)
            ERR_print_errors_fp(stderr);

    SOCKET socket = ::socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(443);
    socklen_t addrlen = sizeof(addr);
    if (bind(socket, (struct sockaddr*)&addr, addrlen) < 0)
        printf("could not bind port 443.\n");

    const std::string server_data =
        std::format(
            "server|{}\n"
            "port|{}\n"
            "type|1\n"
            "type2|1\n"
            "#maint|Server under maintenance. Please try again later.\n"
            "loginurl|login-web-sigma.vercel.app\n"
            "meta|gurotopia\n"
            "RTENDMARKERBS1001", ip, enet_port);
    const std::string response =
        std::format(
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: {}\r\n"
            "Connection: close\r\n"
            "\r\n{}",
            server_data.size(), server_data).c_str();

    listen(socket, 9);
    while (true)
    {
        SOCKET fd = accept(socket, (struct sockaddr*)&addr, &addrlen);
        if (fd < 0) continue;

        char ip[INET_ADDRSTRLEN]; // @note allocated LPSTR 
        inet_ntop(AF_INET, &addr.sin_addr, ip, INET_ADDRSTRLEN);

        https::client &client = clients[std::string{ip}];

        if (steady_clock::now() - client.last_connect >= 3s)
        {
            SSL *ssl = SSL_new(ctx);
            SSL_set_fd(ssl, fd);
            if (SSL_accept(ssl) > 0)
            {
                char buf[213]; // @note size of growtopia's request.
                int length{ sizeof(buf ) }; // @note openssl is written in C so this will act as std::string::length()

                if (SSL_read(ssl, buf, length) == length)
                {
                    if (std::string_view{buf, sizeof(buf )}.contains("POST /growtopia/server_data.php HTTP/1.1"))
                        SSL_write(ssl, response.c_str(), response.size());
                }
                else ERR_print_errors_fp(stderr);
            }
            else ERR_print_errors_fp(stderr);
            SSL_free(ssl);
        }
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