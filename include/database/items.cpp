#include "pch.hpp"
#include "items.hpp"

std::unordered_map<unsigned short, item> items;
std::vector<std::byte> im_data(61, std::byte{ 00 });

template<typename T>
void shift_pos(std::vector<std::byte>& data, unsigned& pos, T& value) 
{
    for (std::size_t i = 0zu; i < sizeof(T); ++i) 
        reinterpret_cast<std::byte*>(&value)[i] = data[pos + i];
    pos += sizeof(T);
}

/* have not tested modifying string values... */
template<typename T>
void data_modify(std::vector<std::byte>& data, unsigned& pos, const T& value) 
{
    for (std::size_t i = 0zu; i < sizeof(T); ++i) 
        data[pos + i] = reinterpret_cast<const std::byte*>(&value)[i];
}

void cache_items()
{
    unsigned pos{60};
    short version{};
    shift_pos(im_data, pos, version);
#if defined(_MSC_VER)
    printf("items.dat %d\n", version);
#else
    printf("\e[38;5;248mitems.dat \e[1;37m%d\e[0m\n", version);
#endif
    short count{};
    shift_pos(im_data, pos, count); pos += 2; // @note downside count to 2 bit (short)
    static constexpr std::string_view token{"PBG892FXX982ABC*"};
    for (unsigned short i = 0; i < count; ++i)
    {
        item im{};
        
        shift_pos(im_data, pos, im.id); pos += 2; // @note downside im.id to 2 bit (short)
        pos += 1;
        shift_pos(im_data, pos, im.cat);
        shift_pos(im_data, pos, im.type);
        pos += 1;

        short len = *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short);
        im.raw_name.resize(len);
        for (short i = 0; i < len; ++i) 
            im.raw_name[i] = std::to_integer<char>(im_data[pos] ^ std::byte(token[(i + im.id) % token.length()])), 
            ++pos;

        len = *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short) + len;

        pos += 13;
        shift_pos(im_data, pos, im.collision);
        {
            std::byte raw_hits{};
            shift_pos(im_data, pos, raw_hits);
            im.hits = std::to_integer<short>(raw_hits);
            if (im.hits != 0) im.hits /= 6; // @note unknown reason behind why break hit is muliplied by 6 then having to divide by 6
        } // @note delete raw_hits
        shift_pos(im_data, pos, im.hit_reset);

        if (im.type == std::byte {type::CLOTHING}) 
        {
            std::byte cloth_type{};
            shift_pos(im_data, pos, cloth_type);
            im.cloth_type = std::to_integer<unsigned short>(cloth_type);
        }
        else pos += 1; // @note assign nothing
        if (im.type == std::byte{type::AURA}) im.cloth_type = clothing::ances;
        shift_pos(im_data, pos, im.rarity);

        pos += 1;

        len = *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short);
        im.audio_directory.resize(len);
        for (short i = 0; i < len; ++i) 
            im.audio_directory += std::to_integer<char>(im_data[pos]), 
            ++pos;

        if (im.audio_directory.ends_with(".mp3"))
            data_modify(im_data, pos, 0); // @todo make it only for IOS
        shift_pos(im_data, pos, im.audioHash);

        pos += 4;

        len = *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short) + len;

        len = *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short) + len;

        len = *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short) + len;

        len = *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short) + len;

        pos += 16;

        shift_pos(im_data, pos, im.tick);

        shift_pos(im_data, pos, im.mod);
        shift_pos(im_data, pos, im.mod1);

        len = *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short) + len;

        len = *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short) + len;

        len = *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short) + len;

        pos += sizeof(std::array<std::byte, 80zu>);
        if (version >= 11)
        {
            pos += *(reinterpret_cast<short*>(&im_data[pos]));
            pos += sizeof(short);
        }
        if (version >= 12)
        {
            shift_pos(im_data, pos, im.mod2);
            pos += sizeof(std::array<std::byte, 9zu>);
        }
        if (version >= 13)
            shift_pos(im_data, pos, im.mod3);
        if (version >= 14)
            shift_pos(im_data, pos, im.mod4);
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
        if (version >= 17)
            shift_pos(im_data, pos, im.mod5);
        if (version >= 18)
            shift_pos(im_data, pos, im.mod6);
        if (version >= 19)
            pos += sizeof(std::array<std::byte, 9zu>);
        if (version == 21)
            shift_pos(im_data, pos, im.mod7);
        
        items.emplace(i, im);
    }
}
