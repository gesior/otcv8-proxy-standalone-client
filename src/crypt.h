#include <cstdint>
#include <cstring>
#include "rsa.h"

inline uint32_t adler32(const uint8_t* buffer, size_t size)
{
    size_t a = 1, b = 0, tlen;
    while (size>0) {
        tlen = size>5552 ? 5552 : size;
        size -= tlen;
        do {
            a += *buffer++;
            b += a;
        }
        while (--tlen);

        a %= 65521;
        b %= 65521;
    }
    return (b << 16) | a;
}

inline void rsa_decrypt(uint8_t* buffer)
{
    static RSA g_RSA;
    static bool intialized = false;
    if (!intialized) {
        const char* p(
                "14299623962416399520070177382898895550795403345466153217470516082934737582776038882967213386204600674145392845853859217990626450972452084065728686565928113");
        const char* q(
                "7630979195970404721891201847792002125535401292779123937207447574596692788513647179235335529307251350570728407373705564708871762033017096809910315212884101");
        intialized = true;
        g_RSA.setKey(p, q);
    }
    g_RSA.decrypt((char*) buffer);
}

inline void xtea_encrypt(uint8_t* buffer, uint32_t size, uint32_t* k)
{
    const uint32_t delta = 0x61C88647;

    size_t readPos = 0;
    while (readPos<size) {
        uint32_t v0;
        memcpy(&v0, buffer+readPos, 4);
        uint32_t v1;
        memcpy(&v1, buffer+readPos+4, 4);

        uint32_t sum = 0;

        for (int32_t i = 32; --i>=0;) {
            v0 += ((v1 << 4 ^ v1 >> 5)+v1) ^ (sum+k[sum & 3]);
            sum -= delta;
            v1 += ((v0 << 4 ^ v0 >> 5)+v0) ^ (sum+k[(sum >> 11) & 3]);
        }

        memcpy(buffer+readPos, &v0, 4);
        readPos += 4;
        memcpy(buffer+readPos, &v1, 4);
        readPos += 4;
    }
}

inline void xtea_decrypt(uint8_t* buffer, uint32_t size, uint32_t* k)
{
    const uint32_t delta = 0x61C88647;
    size_t readPos = 0;
    while (readPos<size) {
        uint32_t v0;
        memcpy(&v0, buffer+readPos, 4);
        uint32_t v1;
        memcpy(&v1, buffer+readPos+4, 4);

        uint32_t sum = 0xC6EF3720;

        for (int32_t i = 32; --i>=0;) {
            v1 -= ((v0 << 4 ^ v0 >> 5)+v0) ^ (sum+k[(sum >> 11) & 3]);
            sum += delta;
            v0 -= ((v1 << 4 ^ v1 >> 5)+v1) ^ (sum+k[sum & 3]);
        }

        memcpy(buffer+readPos, &v0, 4);
        readPos += 4;
        memcpy(buffer+readPos, &v1, 4);
        readPos += 4;
    }
}