#include "pch.hpp"
#include "items.hpp"

std::unordered_map<u_short, item> items;
std::vector<u_char> im_data(sizeof(::state)/*inital packet*/ + 1, 0x00);

template<typename T>
void shift_pos(const std::vector<u_char> &data, u_int &pos, T &value) 
{
    u_char *_1bit = reinterpret_cast<u_char*>(&value); 
    for (std::size_t i = 0zu; i < sizeof(T); ++i) 
        _1bit[i] = data[pos + i];
    pos += sizeof(T);
}

/* have not tested modifying string values··· */
template<typename T>
void data_modify(std::vector<u_char> &data, const u_int &pos, const T &value) 
{
    const u_char *_1bit = reinterpret_cast<const u_char*>(&value);
    for (std::size_t i = 0zu; i < sizeof(T); ++i) 
        data[pos + i] = _1bit[i];
}

void cache_items()
{
    u_int pos{ sizeof(::state) };
    u_char version{};
    shift_pos(im_data, pos, version); pos += 1; // @note downsize 'version' to 1 bit
    u_short count{};
    shift_pos(im_data, pos, count); pos += 2; // @note downside count to 2 bit
    static constexpr std::string_view token{"PBG892FXX982ABC*"};
    for (u_short i = 0; i < count; ++i)
    {
        item im{};
        
        shift_pos(im_data, pos, im.id); pos += 2; // @note downside im.id to 2 bit (short)
        shift_pos(im_data, pos, im.property);

        shift_pos(im_data, pos, im.cat);

        shift_pos(im_data, pos, im.type);
        pos += sizeof(u_char);

        short len = *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short);
        im.raw_name.resize(len);
        for (short i = 0; i < len; ++i) 
            im.raw_name[i] = im_data[pos] ^ token[(i + im.id) % token.length()], 
            ++pos;

        pos += *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short);

        pos += sizeof(std::array<u_char, 13zu>);

        shift_pos(im_data, pos, im.collision);
        shift_pos(im_data, pos, im.hits);
        if (im.hits != 0) im.hits /= 6; // @note unknown reason behind why break hit is muliplied by 6 then having to divide by 6

        shift_pos(im_data, pos, im.hit_reset);

        if (im.type == type::CLOTHING) 
        {
            u_char cloth_type{};
            shift_pos(im_data, pos, im.cloth_type);
        }
        else pos += 1; // @note assign nothing
        if (im.type == type::AURA) im.cloth_type = clothing::ances;
        shift_pos(im_data, pos, im.rarity);

        pos += sizeof(u_char);
        {
            len = *reinterpret_cast<short*>(&im_data[pos]);
            pos += sizeof(short);
            std::string audio_directory{};
            audio_directory.assign(reinterpret_cast<char*>(&im_data[pos]), len);
            pos += len;

            if (audio_directory.ends_with(".mp3"))
                data_modify(im_data, pos, 0); // @todo make it only for IOS
        }
        pos += sizeof(int);

        pos += sizeof(std::array<u_char, 4zu>);

        pos += *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short);

        pos += *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short);

        pos += *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short);

        pos += *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short);

        pos += sizeof(std::array<u_char, 16zu>);

        shift_pos(im_data, pos, im.tick);

        pos += sizeof(short);
        pos += sizeof(short);

        pos += *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short);

        pos += *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short);

        pos += *(reinterpret_cast<short*>(&im_data[pos]));
        pos += sizeof(short);

        pos += sizeof(std::array<u_char, 80zu>);

        if (version >= 11) // @date February 2019
        {
            pos += *(reinterpret_cast<short*>(&im_data[pos]));
            pos += sizeof(short);
        }
        if (version >= 12) // @date October 2020
        {
            pos += sizeof(int);
            pos += sizeof(std::array<u_char, 9zu>);
        }
        if (version >= 13) pos += sizeof(int); // @date May 2021
        if (version >= 14) pos += sizeof(int); // @date October 2021
        if (version >= 15)
        {
            pos += sizeof(std::array<u_char, 25zu>);
            pos += *(reinterpret_cast<short*>(&im_data[pos]));
            pos += sizeof(short);
        }
        if (version >= 16)
        {
            pos += *(reinterpret_cast<short*>(&im_data[pos]));
            pos += sizeof(short);
        }
        if (version >= 17) pos += sizeof(int); // @date April 2024
        if (version >= 18) pos += sizeof(int); // @date December 2024
        if (version >= 19) pos += sizeof(std::array<u_char, 9zu>);
        if (version >= 21) pos += sizeof(short); // @date September 2025
        if (version >= 22)
        {
            len = *reinterpret_cast<short*>(&im_data[pos]);
            pos += sizeof(short);
            im.info.assign(reinterpret_cast<char*>(&im_data[pos]), len);
            pos += len;
        }
        if (version >= 23) 
        {
            shift_pos(im_data, pos, im.splice[0]);
            shift_pos(im_data, pos, im.splice[1]);
        }
        if (version == 24) pos += sizeof(u_char); // @date December 2025

        items.emplace(i, im);
    }
    std::printf("parsed %zu items; %zu KB of stack memory\n", items.size(), (items.size() * sizeof(item)) / 1024);
}
