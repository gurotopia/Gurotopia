#include "pch.hpp"
#include "packet.hpp"

void packet::create(ENetPeer& p, bool netid, signed delay, const std::vector<std::any>& params) 
{
    ::peer *_p = static_cast<::peer*>(p.data);

    std::vector<u_char> data = compress_state(::state{
        .type = 01,
        .netid = (!netid) ? -1 : _p->netid,
        .peer_state = 0x08,
        .id = delay
    });

    std::size_t size = data.size();
    u_char index{};
    for (const std::any &param : params) 
    {
        if (param.type() == typeid(const char*)) 
        {
            std::string_view param_view{ std::any_cast<const char*>(param) };
            data.resize(size + 2zu + sizeof(int) + param_view.length());
            data[size] = index; // @note element counter e.g. "OnConsoleMessage" -> 00, "hello" -> 01
            data[size + 1zu] = 0x02;
            *reinterpret_cast<int*>(&data[size + 2zu]) = param_view.length();

            const u_char *_1bit = reinterpret_cast<const u_char*>(param_view.data());
            for (std::size_t i = 0zu; i < param_view.length(); ++i)
                data[size + 6zu + i] = _1bit[i]; // @note e.g. 'a' -> 0x61. 'z' = 0x7A, hex tabel: https://en.cppreference.com/w/cpp/language/ascii

            size += 2zu + sizeof(int) + param_view.length();
        }
        else if (param.type() == typeid(int) || param.type() == typeid(u_int)) 
        {
            bool is_signed = (param.type() == typeid(int));
            auto value = is_signed ? std::any_cast<int>(param) : std::any_cast<u_int>(param);
            data.resize(size + 2zu + sizeof(value));
            data[size] = index; // @note element counter e.g. "OnSetBux" -> 00, 43562/-43562 -> 01
            data[size + 1zu] = (is_signed) ? 0x09 : 0x05;

            const u_char *_1bit = reinterpret_cast<const u_char*>(&value);
            for (std::size_t i = 0zu; i < sizeof(value); ++i)
                data[size + 2zu + i] = _1bit[i];

            size += 2zu + sizeof(value);
        }
        else if (param.type() == typeid(std::vector<float>)) 
        {
            const std::vector<float>& vec = std::any_cast<const std::vector<float>&>(param);
            data.resize(size + 2zu + (sizeof(float) * vec.size()));
            data[size] = index;
            data[size + 1zu] = 
                (vec.size() == 1zu) ? 0x01 :
                (vec.size() == 2zu) ? 0x03 :
                (vec.size() == 3zu) ? 0x04 : 0x00;

            const u_char *_1bit = reinterpret_cast<const u_char*>(vec.data());
            for (std::size_t i = 0zu; i < sizeof(float) * vec.size(); ++i)
                data[size + 2zu + i] = _1bit[i];

            size += 2zu + (sizeof(float) * vec.size());
        }
        else return; // @note this will never pass unless you include a param that Growtopia does not recognize

        data[60zu] = ++index;
    }

    enet_peer_send(&p, 0, enet_packet_create(data.data(), size, ENET_PACKET_FLAG_RELIABLE));
}

void packet::action(ENetPeer& p, const std::string& action, const std::string& str) 
{
    std::string_view action_view = std::format("action|{}\n", action);
    std::vector<u_char> data(4zu + action_view.length() + str.length(), 0x00);
    data[0zu] = 0x03;
    {
        const u_char *_1bit = reinterpret_cast<const u_char*>(action_view.data());
        for (std::size_t i = 0zu; i < action_view.length(); ++i)
            data[4zu + i] = _1bit[i];
    }
    if (!str.empty())
    {
        const u_char *_1bit = reinterpret_cast<const u_char*>(str.data());
        for (std::size_t i = 0zu; i < str.length(); ++i)
            data[4zu + action_view.length() + i] = _1bit[i];
    }
    
    enet_peer_send(&p, 0, enet_packet_create(data.data(), data.size(), ENET_PACKET_FLAG_RELIABLE));
}