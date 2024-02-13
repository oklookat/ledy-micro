#pragma once

#include <array>
#include <EEPROM.h>

unsigned short bytesToUint16(unsigned char byte1, unsigned char byte2)
{
    return (byte1 << 8) | byte2;
}

unsigned int bytesToUint32(unsigned char byte1, unsigned char byte2, unsigned char byte3, unsigned char byte4)
{
    unsigned int result = 0;
    result |= static_cast<unsigned int>(byte1) << 24;
    result |= static_cast<unsigned int>(byte2) << 16;
    result |= static_cast<unsigned int>(byte3) << 8;
    result |= static_cast<unsigned int>(byte4);
    return result;
}

void copyAndCheckTerminator(const uint8_t *src, std::array<uint8_t, 32> &dest)
{
    std::copy(src, src + 32, dest.begin());
    if (std::find(dest.begin(), dest.end(), 0) == dest.end())
    {
        // Add null term. if not exists.
        dest[31] = 0;
    }
}

void write32EEPROM(const std::array<uint8_t, 32> &data, int address)
{
    if (address < 0 || address > EEPROM.length() - 32) // Проверка на корректность адреса
        return;

    EEPROM.begin(address + 33);
    for (auto i = address; i < address + 32; ++i)
    {
        if (i - address < data.size())
        {
            EEPROM.write(i, data[i - address]);
        }
        else
        {
            EEPROM.write(i, 0); // Записываем нулевой терминатор после строки
        }
    }
    EEPROM.commit();
}

std::string getString32EEPROM(int address)
{
    if (address < 0 || address > EEPROM.length() - 32) // Проверка на корректность адреса
        return "";

    EEPROM.begin(address + 33);
    std::string result;
    for (size_t addr = address; addr < address + 32; addr++)
    {
        unsigned char uChar = EEPROM.readUChar(addr);
        if (uChar == 0)
        {
            break;
        }
        result.push_back(static_cast<char>(uChar));
    }
    return result;
}

std::string getSSID()
{
    return getString32EEPROM(0);
}

std::string getPassword()
{
    return getString32EEPROM(32);
}
