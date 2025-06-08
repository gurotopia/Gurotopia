#include "pch.hpp"
#include "items.hpp"
#include "peer.hpp"
#include "world.hpp"

#include "nlohmann/json.hpp" // @note https://github.com/nlohmann/json

int peer::emplace(slot s) 
{
    if (auto it = std::find_if(slots.begin(), slots.end(), [&](const slot &found) { return found.id == s.id; }); it != slots.end()) 
    {
        int excess = std::max(0, (it->count + s.count) - 200);
        it->count = std::min(it->count + s.count, 200);
        if (it->count == 0)
        {
            item &item = items[it->id];
            if (item.cloth_type != clothing::none) this->clothing[item.cloth_type] = 0;
        }
        return excess;
    }
    else slots.emplace_back(std::move(s)); // @note no such item in inventory, so we create a new entry.
    return 0;
}

void peer::add_xp(unsigned short value) 
{
    this->level.back() += value;

    unsigned short lvl = this->level.front();
    unsigned short xp_formula = 50 * (lvl * lvl + 2); // @note credits: https://www.growtopiagame.com/forums/member/553046-kasete
    
    unsigned short level_up = std::min<unsigned short>(this->level.back() / xp_formula, 125 - lvl);
    this->level.front() += level_up;
    this->level.back() -= level_up * xp_formula;
}

peer& peer::read(const std::string& name)
{
    std::ifstream file(std::format("players\\{}.json", name));
    if (file.is_open()) 
    {
        nlohmann::json j;
        file >> j;
        this->role = j.contains("role") && !j["role"].is_null() ? j["role"].get<char>() : this->role;
        this->gems = j.contains("gems") && !j["gems"].is_null() ? j["gems"].get<int>() : this->gems;
        this->level = j.contains("level") && !j["level"].is_null() ? j["level"].get<std::array<unsigned short, 2zu>>() : this->level;
        this->recent_worlds = j.contains("r_worlds") && !j["r_worlds"].is_null() ? j["r_worlds"].get<std::array<std::string, 6zu>>() : this->recent_worlds;
        this->my_worlds = j.contains("my_worlds") && !j["my_worlds"].is_null() ? j["my_worlds"].get<std::array<std::string, 200zu>>() : this->my_worlds;
        this->fav = j.contains("fav") && !j["fav"].is_null() ? j["fav"].get<std::vector<short>>() : this->fav;
        for (const auto& i : j["slots"]) this->slots.emplace_back(slot{ i["i"], i["c"] });
    }
    return *this;
}

peer::~peer()
{
    nlohmann::json j;
    j["role"] = this->role;
    j["gems"] = this->gems;
    j["level"] = this->level;
    j["r_worlds"] = this->recent_worlds;
    j["my_worlds"] = this->my_worlds;
    j["fav"] = this->fav;
    for (const slot &slot : this->slots)
    {
        if ((slot.id == 18 || slot.id == 32) || slot.count <= 0) continue;
        j["slots"].emplace_back(nlohmann::json{{"i", slot.id}, {"c", slot.count}});
    }

    std::ofstream(std::format("players\\{}.json", this->ltoken[0]), std::ios::trunc) << j.dump();
}

std::unordered_map<ENetPeer*, std::shared_ptr<peer>> _peer;

bool create_rt(ENetEvent& event, std::size_t pos, int64_t length) 
{
    auto &rt = _peer[event.peer]->rate_limit[pos];
    auto now = std::chrono::steady_clock::now();

    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - rt).count() <= length)
        return false;

    rt = now;
    return true;
}

ENetHost *server;

std::vector<ENetPeer*> peers(ENetEvent event, peer_condition condition, std::function<void(ENetPeer&)> fun)
{
    std::vector<ENetPeer*> _peers{};
    _peers.reserve(server->peerCount);
    auto &peer_worlds = _peer[event.peer]->recent_worlds;
    for (ENetPeer &peer : std::span(server->peers, server->peerCount))
        if (peer.state == ENET_PEER_STATE_CONNECTED) 
        {
            if (condition == PEER_SAME_WORLD)
            {
                if ((_peer[&peer]->recent_worlds.empty() && peer_worlds.empty()) || 
                    (_peer[&peer]->recent_worlds.back() != peer_worlds.back())) continue;
            }
            fun(peer);
            _peers.push_back(&peer);
        }

    return _peers;
}

state get_state(const std::vector<std::byte>& packet) 
{
    const int *_4bit = reinterpret_cast<const int*>(packet.data());
    const float *_4bit_f = reinterpret_cast<const float*>(packet.data());
    return state{
        .type = _4bit[0],
        .netid = _4bit[1],
        .peer_state = _4bit[3],
        .count = _4bit_f[4],
        .id = _4bit[5],
        .pos = {_4bit_f[6], _4bit_f[7]},
        .speed = {_4bit_f[8], _4bit_f[9]},
        .punch = {_4bit[11], _4bit[12]}
    };
}

std::vector<std::byte> compress_state(const state& s) 
{
    std::vector<std::byte> data(56, std::byte{ 00 });
    int *_4bit = reinterpret_cast<int*>(data.data());
    float *_4bit_f = reinterpret_cast<float*>(data.data());
    _4bit[0] = s.type;
    _4bit[1] = s.netid;
    _4bit[3] = s.peer_state;
    _4bit_f[4] = s.count;
    _4bit[5] = s.id;
    _4bit_f[6] = s.pos[0];
    _4bit_f[7] = s.pos[1];
    _4bit_f[8] = s.speed[0];
    _4bit_f[9] = s.speed[1];
    _4bit[11] = s.punch[0];
    _4bit[12] = s.punch[1];
    return data;
}

void inventory_visuals(ENetEvent &event)
{
    auto &peer = _peer[event.peer];
	std::size_t size = peer->slots.size();
    std::vector<std::byte> data(66zu + (size * sizeof(int)));
    
    data[0zu] = std::byte{ 04 };
    data[4zu] = std::byte{ 0x09 };
    *reinterpret_cast<int*>(&data[8zu]) = peer->netid;
    data[16zu] = std::byte{ 0x08 };
    *reinterpret_cast<int*>(&data[58zu]) = std::byteswap<int>(peer->slot_size);
    *reinterpret_cast<int*>(&data[62zu]) = std::byteswap<int>(size);
    int *slot_ptr = reinterpret_cast<int*>(data.data() + 66);
    for (const slot &slot : peer->slots)
        *slot_ptr++ = (slot.id | (slot.count << 16)) & 0x00ffffff;

	enet_peer_send(event.peer, 0, enet_packet_create(data.data(), data.size(), ENET_PACKET_FLAG_RELIABLE));
    clothing_visuals(event); // @todo
}
