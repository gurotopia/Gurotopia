#include "pch.hpp"
#include <filesystem>
#include <fstream>

#include "items.hpp"

std::vector<item> items;

std::vector<u_char> im_data(sizeof(::state)/*inital packet*/, 0x00);

template<typename T>
void shift_pos(const std::vector<u_char> &data, u_int &pos, T &value) 
{
    u_char *i8 = reinterpret_cast<u_char*>(&value); 
    for (std::size_t i = 0zu; i < sizeof(T); ++i) 
        i8[i] = data[pos + i];
    pos += sizeof(T);
}

/* have not tested modifying string values··· */
template<typename T>
void data_modify(std::vector<u_char> &data, const u_int &pos, const T &value) 
{
    const u_char *i8 = reinterpret_cast<const u_char*>(&value);
    for (std::size_t i = 0zu; i < sizeof(T); ++i) 
        data[pos + i] = i8[i];
}

void decode_items()
{
    const u_int size = std::filesystem::file_size("items.dat");
    im_data = compress_state(::state{
        .type = 0x10, // @note PACKET_SEND_ITEM_DATABASE_DATA
        .peer_state = 0x08, 
        .size = size
    });
    u_int pos = im_data.size(); // @note sizeof(::state)
    im_data.resize(pos + size); // @note resize to fit binary data
    
    std::ifstream("items.dat", std::ios::binary)
        .read((char*)&im_data[pos], size); // @note the binary data···

    u_short version{};
    shift_pos(im_data, pos, version);
    u_int count{};
    shift_pos(im_data, pos, count);

    const std::string_view token{"PBG892FXX982ABC*"};
    for (u_int i = 0; i < count; ++i)
    {
        ::item im;
        
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

        pos += sizeof(int);
        pos += sizeof(u_char);

        shift_pos(im_data, pos, im.ingredient);
        pos += sizeof(u_char);
        pos += sizeof(u_char);
        pos += sizeof(u_char);
        pos += sizeof(u_char);

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

        if (version >= 0x0b) // @date February 2019
        {
            pos += *(reinterpret_cast<short*>(&im_data[pos]));
            pos += sizeof(short);
        }
        if (version >= 0x0c) // @date October 2020
        {
            pos += sizeof(int);
            pos += sizeof(std::array<u_char, 9zu>);
        }
        if (version >= 0x0d) pos += sizeof(int); // @date May 2021
        if (version >= 0x0e) pos += sizeof(int); // @date October 2021
        if (version >= 0x0f)
        {
            pos += sizeof(std::array<u_char, 25zu>);
            pos += *(reinterpret_cast<short*>(&im_data[pos]));
            pos += sizeof(short);
        }
        if (version >= 0x10)
        {
            pos += *(reinterpret_cast<short*>(&im_data[pos]));
            pos += sizeof(short);
        }
        if (version >= 0x11) pos += sizeof(int); // @date April 2024
        if (version >= 0x12) pos += sizeof(int); // @date December 2024
        if (version >= 0x13) pos += sizeof(std::array<u_char, 9zu>);
        if (version >= 0x15) pos += sizeof(short); // @date September 2025
        if (version >= 0x16)
        {
            len = *reinterpret_cast<short*>(&im_data[pos]);
            pos += sizeof(short);
            im.info.assign(reinterpret_cast<char*>(&im_data[pos]), len);
            pos += len;
        }
        if (version >= 0x17) 
        {
            shift_pos(im_data, pos, im.splice[0]);
            shift_pos(im_data, pos, im.splice[1]);
        }
        if (version >= 0x18) pos += sizeof(u_char); // @date December 2025
        if (version >= 0x19)
        {
            len = *reinterpret_cast<short*>(&im_data[pos]);
            pos += sizeof(short);
            if (len > (sizeof(short) + sizeof(int))) // @note {size} 0x00 0x00 0x00 0x00
            {
                // @todo add the string for the player punch FX
                pos += len;
            }
            pos += sizeof(int); // @note default: 0x00 0x00 0x00 0x00
        }
        if (version == 0x1a) pos += sizeof(std::byte); // May 2026
        
        items.emplace_back(im);
    }
    printf("items.dat parsed successfully!\n");
}
