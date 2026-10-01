#include "rsa.h"
#include <string.h>
#include <stdio.h>

RSA::RSA()
{
	mpz_init(m_n);
	mpz_init2(m_d, 1024);
}

RSA::~RSA()
{
	mpz_clear(m_n);
	mpz_clear(m_d);
}

void RSA::setKey(const char* p, const char* q)
{
	std::lock_guard<std::recursive_mutex> lockClass(lock);

	mpz_t m_p, m_q;
	mpz_init2(m_p, 1024);
	mpz_init2(m_q, 1024);
	mpz_init(m_e);

	mpz_set_str(m_p, p, 10);
	mpz_set_str(m_q, q, 10);

	// e = 65537
	mpz_set_ui(m_e, 65537);

	// n = p * q
	mpz_mul(m_n, m_p, m_q);

	// d = e^-1 mod (p - 1)(q - 1)
	mpz_t p_1, q_1, pq_1;
	mpz_init2(p_1, 1024);
	mpz_init2(q_1, 1024);
	mpz_init2(pq_1, 1024);

	mpz_sub_ui(p_1, m_p, 1);
	mpz_sub_ui(q_1, m_q, 1);

	// pq_1 = (p -1)(q - 1)
	mpz_mul(pq_1, p_1, q_1);

	// m_d = m_e^-1 mod (p - 1)(q - 1)
	mpz_invert(m_d, m_e, pq_1);

	mpz_clear(p_1);
	mpz_clear(q_1);
	mpz_clear(pq_1);

	mpz_clear(m_p);
	mpz_clear(m_q);
}

void RSA::decrypt(char* msg)
{
	std::lock_guard<std::recursive_mutex> lockClass(lock);

	mpz_t c, m;
	mpz_init2(c, 1024);
	mpz_init2(m, 1024);
    
	mpz_import(c, 128, 1, 1, 0, 0, msg);

	// m = c^d mod n
	mpz_powm(m, c, m_d, m_n);

	size_t count = (mpz_sizeinbase(m, 2) + 7)/8;
	memset(msg, 0, 128 - count);
	mpz_export(&msg[128 - count], nullptr, 1, 1, 0, 0, m);

	mpz_clear(c);
	mpz_clear(m);
}

void RSA::encrypt(char* msg)
{
	std::lock_guard<std::recursive_mutex> lockClass(lock);
	mpz_t m_p;
	mpz_init2(m_p, 1024);
	// OTSERV
	// const char* p = "109120132967399429278860960508995541528237502902798129123468757937266291492576446330739696001110603907230888610072655818825358503429057592827629436413108566029093628212635953836686562675849720620786279431090218017681061521755056710823876476444260558147179707119674283982419152118103759076030616683978566631413";
	// CIPSOFT
	const char* p = "132127743205872284062295099082293384952776326496165507967876361843343953435544496682053323833394351797728954155097012103928360786959821132214473291575712138800495033169914814069637740318278150290733684032524174782740134357629699062987023311132821016569775488792221429527047321331896351555606801473202394175817";
	mpz_set_str(m_p, p, 10);

	mpz_t c, m;
	mpz_init2(c, 1024);
	mpz_init2(m, 1024);
    
	mpz_import(c, 128, 1, 1, 0, 0, msg);

	// m = c^d mod n
	mpz_powm(m, c, m_e, m_p);

	size_t count = (mpz_sizeinbase(m, 2) + 7)/8;
	memset(msg, 0, 128 - count);
	mpz_export(&msg[128 - count], nullptr, 1, 1, 0, 0, m);

	mpz_clear(c);
	mpz_clear(m);
}

void RSA::encrypt_otserv(char* msg)
{
	std::lock_guard<std::recursive_mutex> lockClass(lock);
	mpz_t m_p;
	mpz_init2(m_p, 1024);
	// OTSERV
	const char* p = "109120132967399429278860960508995541528237502902798129123468757937266291492576446330739696001110603907230888610072655818825358503429057592827629436413108566029093628212635953836686562675849720620786279431090218017681061521755056710823876476444260558147179707119674283982419152118103759076030616683978566631413";
	// CIPSOFT
	// const char* p = "132127743205872284062295099082293384952776326496165507967876361843343953435544496682053323833394351797728954155097012103928360786959821132214473291575712138800495033169914814069637740318278150290733684032524174782740134357629699062987023311132821016569775488792221429527047321331896351555606801473202394175817";
	mpz_set_str(m_p, p, 10);

	mpz_t c, m;
	mpz_init2(c, 1024);
	mpz_init2(m, 1024);

	mpz_import(c, 128, 1, 1, 0, 0, msg);

	// m = c^d mod n
	mpz_powm(m, c, m_e, m_p);

	size_t count = (mpz_sizeinbase(m, 2) + 7)/8;
	memset(msg, 0, 128 - count);
	mpz_export(&msg[128 - count], nullptr, 1, 1, 0, 0, m);

	mpz_clear(c);
	mpz_clear(m);
}
