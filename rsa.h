#include <gmp.h>
#include <thread>
#include <mutex>
#include <iostream>

class RSA {
public:
    RSA();

    ~RSA();

    void setKey(const char* p, const char* q);

    void decrypt(char* msg);

    void encrypt(char* msg);

    void encrypt_otserv(char* msg);

protected:
    std::recursive_mutex lock;

    //use only GMP
    mpz_t m_n, m_d, m_e;
};
