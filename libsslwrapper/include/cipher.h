#ifndef _SNF_CIPHER_H_
#define _SNF_CIPHER_H_

#include "sslfcn.h"

namespace snf {
namespace ssl {

class cipher
{
private:
	const EVP_CIPHER      *m_cipher = nullptr;

public:
	cipher(const std::string &);
	cipher(const cipher &) = delete;
	cipher(cipher &&) = delete;
	~cipher() {}

	cipher & operator=(const cipher &) = delete;
	cipher & operator=(cipher &&) = delete;

	operator const EVP_CIPHER * () const { return m_cipher; }

	size_t block_size() const;
	size_t key_length() const;
	size_t iv_length() const;
};

} // namespace ssl
} // namespace snf

#endif // _SNF_CIPHER_H_
