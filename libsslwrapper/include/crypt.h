#ifndef _SNF_CRYPT_H_
#define _SNF_CRYPT_H_

#include "sslfcn.h"
#include "safestr.h"
#include "cipher.h"

namespace snf {
namespace ssl {

class crypt
{
protected:
	EVP_CIPHER_CTX  *m_ctx;

public:
	crypt();
	virtual ~crypt();
	virtual void process(uint8_t *, size_t *, const uint8_t *, size_t, bool) = 0;
	virtual size_t safe_process_buf_size(size_t) const = 0;
};

class encrypt : public crypt
{
private:
	size_t  m_blksize = 0;

public:
	encrypt(const cipher &, const safestr &, const safestr &);
	virtual ~encrypt() {}
	size_t process(uint8_t *, size_t, const uint8_t *, size_t, bool last = false);
	size_t safe_process_buf_size(size_t in) { return in + m_blksize; };
};

class decrypt : public crypt
{
public:
	decrypt(const cipher &, const safestr &, const safestr &);
	virtual ~decrypt() {}
	size_t process(uint8_t *, size_t, const uint8_t *, size_t, bool last = false);
	size_t safe_process_buf_size(size_t in) { return in; };
};

} // namespace ssl
} // namespace snf

#endif // _SNF_CRYPT_H_
