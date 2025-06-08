#include "pch.hpp"
#include "items.hpp"
#include "peer.hpp"
#include "network/packet.hpp"
#include "world.hpp"

#include "nlohmann/json.hpp" // @note https://github.com/nlohmann/json

world::world(const std::string& name)
{
    std::ifstream file(std::format("worlds\\{}.json", name));
    if (file.is_open()) 
    {
        nlohmann::json j;
        file >> j;
        this->name = name;
        this->owner = j.contains("owner") && !j["owner"].is_null() ? j["owner"].get<int>() : 00;
        this->ifloat_uid = j.contains("fs_uid") && !j["fs_uid"].is_null() ? j["fs_uid"].get<std::size_t>() : 0zu;
        
        for (const auto& i : j["bs"])
            if (i.contains("f") && i.contains("b"))
                this->blocks.emplace_back(block{ i["f"], i["b"], j.value("l", "") });
        for (const auto& i : j["fs"]) 
            if (i.contains("u") && i.contains("i") && i.contains("c") && i.contains("p") && i["p"].is_array() && i["p"].size() == 2)
                this->ifloats.emplace_back(ifloat{ i["u"], i["i"], i["c"], { i["p"][0], i["p"][1] } });
    }
}

world::~world()
{
    if (!this->name.empty())
    {
        nlohmann::json j;
        if (this->owner != 0) j["owner"] = this->owner;
        if (this->ifloat_uid != 0) j["fs_uid"] = this->ifloat_uid;
        for (const block &block : this->blocks) 
        {
            nlohmann::json list = {{"f", block.fg}, {"b", block.bg}};
            if (!block.label.empty()) list["l"] = block.label;
            j["bs"].push_back(list);
        }
        for (const ifloat &ifloat : this->ifloats) 
        {
            if (ifloat.id == 0 || ifloat.count == 0) continue;
            j["fs"].push_back({{"u", ifloat.uid}, {"i", ifloat.id}, {"c", ifloat.count}, {"p", ifloat.pos}});
        }

        std::ofstream(std::format("worlds\\{}.json", this->name), std::ios::trunc) << j;
    }
}

std::unordered_map<std::string, world> worlds;

void send_data(ENetPeer& peer, const std::vector<std::byte>& data)
{
    std::size_t size = data.size();
    if (size < 14zu) return;
    ENetPacket *packet = enet_packet_create(nullptr, size + 5zu, ENET_PACKET_FLAG_RELIABLE);
    if (packet == nullptr || packet->dataLength < size + 4zu) return;
    packet->data[0zu] = { 04 };
    std::memcpy(packet->data + 4, data.data(), size);
    if (size >= 13zu + sizeof(std::size_t)) 
    {
        std::size_t forecast = std::bit_cast<std::size_t>(data.data() + 13);
        if ((std::to_integer<unsigned char>(data[12zu]) & 0x8) && 
            forecast <= 512zu && packet->dataLength + forecast <= 512zu) 
        {
            enet_packet_resize(packet, packet->dataLength + forecast);
        }
    }
    enet_peer_send(&peer, 1, packet);
}

void state_visuals(ENetEvent& event, state s) 
{
    s.netid = _peer[event.peer]->netid;
    peers(event, PEER_SAME_WORLD, [&](ENetPeer& p) 
    {
        send_data(p, compress_state(s));
    });
}

void block_punched(ENetEvent& event, state s, block &block)
{
    (block.fg == 0) ? ++block.hits[1] : ++block.hits[0];
    s.type = 0x8; // @note PACKET_TILE_APPLY_DAMAGE
    s.id = 6; // @note idk exactly
	state_visuals(event, s);
}

void drop_visuals(ENetEvent& event, const std::array<short, 2zu>& im, const std::array<float, 2zu>& pos, signed uid) 
{
    std::vector<std::byte> compress{};
    state s{.type = 0x0e}; // @note PACKET_ITEM_CHANGE_OBJECT
    if (im[1] == 0 || im[0] == 0)
    {
        s.netid = _peer[event.peer]->netid;
        s.peer_state = -1;
        s.id = uid;
        s.pos = {0.0f, 0.0f};
    }
    else
    {
        world &world = worlds[_peer[event.peer]->recent_worlds.back()];
        std::size_t uid = world.ifloat_uid++;
        std::vector<ifloat> &ifloats{world.ifloats};
        ifloat it = ifloats.emplace_back(ifloat{uid, im[0], im[1], pos}); // @note a iterator ahead of time
        s.netid = -1;
        s.peer_state = static_cast<int>(it.uid);
        s.count = static_cast<float>(im[1]);
        s.id = it.id;
        s.pos = {it.pos[0] * 32, it.pos[1] * 32};
    }
    compress = compress_state(s);
    peers(event, PEER_SAME_WORLD, [&](ENetPeer& p)  
    {
        send_data(p, compress);
    });
}

void clothing_visuals(ENetEvent &event) 
{
    auto &peer = _peer[event.peer];
    gt_packet(*event.peer, true, 0, {
        "OnSetClothing", 
        std::vector<float>{peer->clothing[hair], peer->clothing[shirt], peer->clothing[legs]}, 
        std::vector<float>{peer->clothing[feet], peer->clothing[face], peer->clothing[hand]}, 
        std::vector<float>{peer->clothing[back], peer->clothing[head], peer->clothing[charm]}, 
        peer->skin_color,
        std::vector<float>{peer->clothing[ances], 0.0f, 0.0f}
    });
}

void tile_update(ENetEvent &event, state s, block &block, world& w) 
{
    s.type = 05; // @note PACKET_SEND_TILE_UPDATE_DATA
    s.peer_state = 0x08;
    std::vector<std::byte> data = compress_state(s);

    short pos = 56;
    data.resize(pos + 8); // @note {2} {2} 00 00 00 00
    *reinterpret_cast<short*>(&data[pos]) = block.fg; pos += sizeof(short);
    *reinterpret_cast<short*>(&data[pos]) = block.bg; pos += sizeof(short);
    pos += sizeof(short); // @todo
    pos += sizeof(short); // @todo (water = 00 04)

    switch (items[block.fg].type)
    {
        case std::byte{ type::DOOR }:
        {
            data[pos - 2] = std::byte{ 01 };
            std::span<const char> label = block.label;
            short len{ static_cast<short>(label.size()) };
            data.resize(pos + 1 + 2 + len + 1); // @note 01 {2} {} 0 0

            data[pos] = std::byte{ 01 }; pos += sizeof(std::byte);
            
            *reinterpret_cast<short*>(&data[pos]) = len; pos += sizeof(short);
            for (const char& c : label) data[pos++] = static_cast<std::byte>(c);

            pos += sizeof(std::byte); // @note '\0'
            break;
        }
        case std::byte{ type::SIGN }:
        {
            data[pos - 2] = std::byte{ 0x19 };
            std::span<const char> label = block.label;
            short len{ static_cast<short>(label.size()) };
            data.resize(pos + 1 + 2 + len + 4); // @note 02 {2} {} ff ff ff ff

            data[pos] = std::byte{ 02 }; pos += sizeof(std::byte);

            *reinterpret_cast<short*>(&data[pos]) = len; pos += sizeof(short);
            for (const char& c : label) data[pos++] = static_cast<std::byte>(c);

            *reinterpret_cast<int*>(&data[pos]) = -1; pos += sizeof(int); // @note ff ff ff ff
            break;
        }
    }

    peers(event, PEER_SAME_WORLD, [&](ENetPeer& p) 
    {
        send_data(p, data);
    });
}