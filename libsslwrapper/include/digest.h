#ifndef _SNF_DIGEST_H_
#define _SNF_DIGEST_H_

#include "sslfcn.h"
#include "safestr.h"
#include <string>

namespace snf {
namespace ssl {

class digest
{
private:
	EVP_MD_CTX      *m_ctx = nullptr;
	const EVP_MD    *m_dgst = nullptr;

public:
	digest(const std::string &);
	digest(const digest &);
	digest(digest &&);
	~digest();

	digest & operator=(const digest &);
	digest & operator=(digest &&);

	void add(const void *, size_t);
	safestr *get();
	size_t length();
};

} // namespace ssl
} // namespace snf

#endif // _SNF_DIGEST_H_
