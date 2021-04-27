#include "translator.h"
#include "sslfcn.h"
#include <memory>

namespace snf {
namespace ssl {

constexpr char s_hex_table[] = {
	'0', '1', '2', '3',
	'4', '5', '6', '7',
	'8', '9', 'a', 'b',
	'c', 'd', 'e', 'f'
};

safestr *
translator::hex2bin(const std::string &hstr)
{
    if ((hstr.length() % 2) != 0)
        throw std::invalid_argument("input hex string is not a multiple of 2");
    
    safestr *ss = DBG_NEW safestr(hstr.length() / 2);
    uint8_t *data = ss->data();

    for (size_t i = 0, j = 0; i < hstr.length(); i += 2, ++j) {
        std::string s = hstr.substr(i, 2);
        data[j] = std::stoi(s, nullptr, 16);
    }

    return ss;
}

std::string
translator::bin2hex(const safestr *ss)
{
    return bin2hex(ss->data(), ss->length());
}

std::string
translator::bin2hex(const uint8_t *bstr, size_t len)
{
	if ((bstr == nullptr) || (len == 0))
		return std::string {};

	std::string s(len * 2, ' ');

	for (size_t i = 0; i < len; ++i) {
		s[2 * i]     = s_hex_table[(bstr[i] & 0xF0) >> 4];
		s[2 * i + 1] = s_hex_table[(bstr[i] & 0x0F)];
	}

	return s;
}

safestr *
translator::base642bin(const std::string &bstr)
{
	size_t len = (bstr.length() / 4) + 3;
	std::unique_ptr<uint8_t []> data(DBG_NEW uint8_t[len]);

	const uint8_t *in = reinterpret_cast<const uint8_t *>(bstr.c_str());
	int inlen = static_cast<int>(bstr.length());
	
	int n = CRYPTO_FCN<p_evp_decode_block>("EVP_DecodeBlock")(data.get(), in, inlen);
	if (n <= 0) {
		std::ostringstream oss;
		oss << "expected roughly " << len << " bytes; got " << n << " bytes";
		throw std::runtime_error(oss.str()); 
	}

	safestr *ss = DBG_NEW safestr(n);
	memcpy (ss->data(), data.get(), n);
	memset(data.get(), 0, n);
	return ss;
}

std::string
translator::bin2base64(const safestr *ss)
{
	return bin2base64(ss->data(), ss->length());
}

std::string
translator::bin2base64(const uint8_t *bstr, size_t len)
{
	int expected_len = (len / 3) * 4;
	if ((len % 3) != 0)
		expected_len += 4;
	expected_len += 1;

	safestr ss { expected_len };

	int n = CRYPTO_FCN<p_evp_encode_block>("EVP_EncodeBlock")(ss.data(), bstr, len);
	if (n != expected_len) {
		std::ostringstream oss;
		oss << "expected " << expected_len << " bytes; got " << n << " bytes";
		throw std::runtime_error(oss.str());
	}

	return std::string { reinterpret_cast<const char *>(ss.data()) };
}

} // namespace ssl
} // namespace snf