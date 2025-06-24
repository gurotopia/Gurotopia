#include "pch.hpp"
#include "packet.hpp"

void gt_packet(ENetPeer& p, bool netid, signed delay, const std::vector<std::any>& params) 
{
    std::vector<std::byte> data(61, std::byte{ 00 });
    data[0zu] = std::byte{ 04 };
    data[4zu] = std::byte{ 01 };
    *reinterpret_cast<signed*>(&data[8zu]) = (!netid) ? -1 : _peer[&p]->netid;
    data[16zu] = std::byte{ 0x08 };
    *reinterpret_cast<signed*>(&data[24zu]) = delay;
    // @note 04 00 00 00 01 00 00 00 {netid} {...8} 08 00 00 00 {...8} {delay}

    std::size_t size = data.size();
    std::byte index{};
    for (const std::any &param : params) 
    {
        if (param.type() == typeid(const char*)) 
        {
            std::string_view param_view{ std::any_cast<const char*>(param) };
            data.resize(size + 2zu + sizeof(int) + param_view.length());
            data[size] = index; // @note element counter e.g. "OnConsoleMessage" -> 00, "hello" -> 01
            data[size + 1zu] = std::byte{ 02 };
            *reinterpret_cast<int*>(&data[size + 2zu]) = param_view.length();

            const std::byte *_1bit = reinterpret_cast<const std::byte*>(param_view.data());
            for (std::size_t i = 0zu; i < param_view.length(); ++i)
                data[size + 6zu + i] = _1bit[i]; // @note e.g. 'a' -> 0x61. 'z' = 0x7A, hex tabel: https://en.cppreference.com/w/cpp/language/ascii

            size += 2zu + sizeof(int) + param_view.length();
        }
        else if (param.type() == typeid(int) || param.type() == typeid(unsigned)) 
        {
            bool is_signed = (param.type() == typeid(int));
            auto value = is_signed ? std::any_cast<int>(param) : std::any_cast<unsigned>(param);
            data.resize(size + 2zu + sizeof(value));
            data[size] = index; // @note element counter e.g. "OnSetBux" -> 00, 43562/-43562 -> 01
            data[size + 1zu] = (is_signed) ? std::byte{ 0x09 } : std::byte{ 05 };

            const std::byte *_1bit = reinterpret_cast<const std::byte*>(&value);
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
                (vec.size() == 1zu) ? std::byte{ 01 } :
                (vec.size() == 2zu) ? std::byte{ 03 } :
                (vec.size() == 3zu) ? std::byte{ 04 } :
                                    std::byte{ 00 };

            const std::byte *_1bit = reinterpret_cast<const std::byte*>(vec.data());
            for (std::size_t i = 0zu; i < sizeof(float) * vec.size(); ++i)
                data[size + 2zu + i] = _1bit[i];

            size += 2zu + (sizeof(float) * vec.size());
        }
        else return; // @note this will never pass unless you include a param that Growtopia does not recognize

        index = static_cast<std::byte>(std::to_integer<char>(index) + 1);
        data[60zu] = index;
    }

    enet_peer_send(&p, 0, enet_packet_create(data.data(), size, ENET_PACKET_FLAG_RELIABLE));
}

void action(ENetPeer& p, const std::string& action, const std::string& str) 
{
    std::string_view action_view = std::format("action|{}\n", action);
    std::vector<std::byte> data(4 + action_view.length() + str.length(), std::byte{ 00 });
    data[0zu] = std::byte{ 03 };
    {
        const std::byte *_1bit = reinterpret_cast<const std::byte*>(action_view.data());
        for (std::size_t i = 0zu; i < action_view.length(); ++i)
            data[4zu + i] = _1bit[i];
    }
    {
        const std::byte *_1bit = reinterpret_cast<const std::byte*>(str.data());
        for (std::size_t i = 0zu; i < str.length(); ++i)
            data[4zu + action_view.length() + i] = _1bit[i];
    }
    
    enet_peer_send(&p, 0, enet_packet_create(data.data(), data.size(), ENET_PACKET_FLAG_RELIABLE));
}