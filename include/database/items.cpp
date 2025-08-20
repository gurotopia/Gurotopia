#include "pch.hpp"
#include "items.hpp"

std::unordered_map<u_short, item> items;
std::vector<std::byte> im_data(61, std::byte{ 00 });

template<typename T>
void shift_pos(std::vector<std::byte>& data, u_int& pos, T& value) 
{
    for (std::size_t i = 0zu; i < sizeof(T); ++i) 
        reinterpret_cast<std::byte*>(&value)[i] = data[pos + i];
    pos += sizeof(T);
}

/* have not tested modifying string values... */
template<typename T>
void data_modify(std::vector<std::byte>& data, u_int& pos, const T& value) 
{
    for (std::size_t i = 0zu; i < sizeof(T); ++i) 
        data[pos + i] = reinterpret_cast<const std::byte*>(&value)[i];
}

void cache_items()
{
    u_int pos{60};
    short version{};
    shift_pos(im_data, pos, version);
    short count{};
    shift_pos(im_data, pos, count); pos += 2; // @note downside count to 2 bit (short)
    static constexpr std::string_view token{"PBG892FXX982ABC*"};
    for (u_short i = 0; i < count; ++i)
    {
        item im{};
        
        shift_pos(im_data, pos, im.id); pos += 2; // @note downside im.id to 2 bit (short)
        shift_pos(im_data, pos, im.property);
        shift_pos(im_data, pos, im.cat);
        shift_pos(im_data, pos, im.type);
        pos += 1;

        short len = *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short);
        im.raw_name.resize(len);
        for (short i = 0; i < len; ++i) 
            im.raw_name[i] = std::to_integer<char>(im_data[pos] ^ std::byte(token[(i + im.id) % token.length()])), 
            ++pos;

        pos += *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short);

        pos += 13;

        shift_pos(im_data, pos, im.collision);
        {
            std::byte raw_hits{};
            shift_pos(im_data, pos, raw_hits);
            im.hits = std::to_integer<short>(raw_hits);
            if (im.hits != 0) im.hits /= 6; // @note unknown reason behind why break hit is muliplied by 6 then having to divide by 6
        } // @note delete raw_hits
        shift_pos(im_data, pos, im.hit_reset);

        if (im.type == type::CLOTHING) 
        {
            std::byte cloth_type{};
            shift_pos(im_data, pos, cloth_type);
            im.cloth_type = std::to_integer<u_short>(cloth_type);
        }
        else pos += 1; // @note assign nothing
        if (im.type == type::AURA) im.cloth_type = clothing::ances;
        shift_pos(im_data, pos, im.rarity);

        pos += 1;

        len = *reinterpret_cast<short*>(&im_data[pos]);
        pos += sizeof(short);

        im.audio_directory.assign(reinterpret_cast<char*>(&im_data[pos]), len);
        pos += len;

        if (im.audio_directory.ends_with(".mp3"))
            data_modify(im_data, pos, 0); // @todo make it only for IOS
        shift_pos(im_data, pos, im.audioHash);

        pos += 4;

        pos += *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short);

        pos += *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short);

        pos += *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short);

        pos += *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short);

        pos += 16;

        shift_pos(im_data, pos, im.tick);

        pos += sizeof(short);
        pos += sizeof(short);

        pos += *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short);

        pos += *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short);

        pos += *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short);

        pos += sizeof(std::array<std::byte, 80zu>);
        if (version >= 11)
        {
            pos += *(reinterpret_cast<short*>(&im_data[pos]));
            pos += sizeof(short);
        }
        if (version >= 12)
        {
            pos += sizeof(int);
            pos += sizeof(std::array<std::byte, 9zu>);
        }
        if (version >= 13) pos += sizeof(int);
        if (version >= 14) pos += sizeof(int);
        if (version >= 15)
        {
            pos += sizeof(std::array<std::byte, 25zu>);
            pos += *(reinterpret_cast<short*>(&im_data[pos]));
            pos += sizeof(short);
        }
        if (version >= 16)
        {
            pos += *(reinterpret_cast<short*>(&im_data[pos]));
            pos += sizeof(short);
        }
        if (version >= 17) pos += sizeof(int);
        if (version >= 18) pos += sizeof(int);
        if (version >= 19)
            pos += sizeof(std::array<std::byte, 9zu>);
        if (version >= 21) pos += sizeof(short);
        if (version == 22)
        {
            short len = *reinterpret_cast<short*>(&im_data[pos]);
            pos += sizeof(short);
            im.info.assign(reinterpret_cast<char*>(&im_data[pos]), len);
            pos += len;
        }
        
        items.emplace(i, im);
    }
}
