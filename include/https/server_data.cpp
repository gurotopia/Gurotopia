#include "pch.hpp"
#include <fstream>
#include <mutex>
#include <unordered_map>

#include "server_data.hpp"

::server_data gServer_data{};

void ::server_data::init()
{
    std::ifstream file("server_data.php");
    if (!file.is_open())
    {
        std::ofstream ostrm("server_data.php");
        ostrm << 
            std::format(
                "server|{}\n"
                "port|{}\n"
                "type|{}\n"
                "type2|{}\n"
                "#maint|{}\n"
                "loginurl|{}\n"
                "meta|{}\n"
                "RTENDMARKERBS1001", 
                this->server, this->port, this->type, this->type2, this->maint, this->loginurl, this->meta
            );
    } // @note close ostrm
    else
    {
        std::vector<std::string> pipes;
        for (std::string line; std::getline(file, line); ) 
        {
            auto pipe_pair = readch(line, '|');
            pipes.insert(pipes.end(), pipe_pair.begin(), pipe_pair.end());
        }

        this->server = pipes[1];
        this->port = std::stoi(pipes[3]);
        this->type = std::stoi(pipes[5]);
        this->type2 = std::stoi(pipes[7]);
        this->maint = pipes[9];
        this->loginurl = pipes[11];
        this->meta = pipes[13];
    } // @note delete str, pipes
} // @note close file


static std::mutex address_mutex{};
static std::unordered_map<std::string, std::string> address_of{}; // @note {player ip, address they were given}

void remember_address(const std::string &client_ip, const std::string &server_ip)
{
    std::lock_guard<std::mutex> lock(address_mutex);
    address_of[client_ip] = server_ip;
}

std::string server_for_peer(const ENetPeer &p)
{
    char buf[64]{};
    if (enet_address_get_host_ip(&p.address, buf, sizeof(buf)) == 0)
    {
        std::string ip = buf;
        if (ip.starts_with("::ffff:")) ip = ip.substr(7);
        if (ip == "127.0.0.1" || ip == "::1") return "127.0.0.1";
        std::lock_guard<std::mutex> lock(address_mutex);
        if (auto it = address_of.find(ip); it != address_of.end()) return it->second;
    }
    std::ifstream f("public_ip.txt"); // @note fallback for players we didn't see at the first step
    std::string pub{};
    std::getline(f, pub);
    while (!pub.empty() && std::isspace(static_cast<unsigned char>(pub.back()))) pub.pop_back();
    return pub.empty() ? gServer_data.server : pub;
}