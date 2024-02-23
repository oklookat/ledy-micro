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