#ifndef _SNF_HTTP_CMN_SCANNER_H_
#define _SNF_HTTP_CMN_SCANNER_H_

#include <string>
#include <vector>
#include <utility>

namespace snf {
namespace http {

using param_vec_t = std::vector<std::pair<std::string, std::string>>;

/*
 * Scanner for HTTP messages.
 */
class scanner
{
private:
	size_t                  m_cur;
	size_t                  m_len;
	const std::string       &m_str;

	int get() { return (m_cur < m_len) ? m_str[m_cur++] : -1; }
	void unget() { if (m_cur < m_len) m_cur--; }
	int peek() { return (m_cur < m_len) ? m_str[m_cur] : -1; }

public:
	scanner() = delete;

	/*
	 * Initializes the scanner with the input string.
	 * @param [in] is - reference to the input stream.
	 */
	scanner(const std::string &s) : m_cur{0}, m_len{s.size()}, m_str{s} {}
	~scanner() {}

	bool read_space();
	bool read_opt_space();
	bool read_special(int);
	bool read_crlf();

	bool read_token(std::string &, bool lower = true);
	bool read_uri(std::string &);
	bool read_version(std::string &);
	bool read_status(std::string &);
	bool read_reason(std::string &);
	bool read_qstring(std::string &);
	bool read_comments(std::string &);
	bool read_parameters(param_vec_t &);
	bool read_all(std::string &);
};

} // namespace http
} // namespace snf

#endif // _SNF_HTTP_CMN_SCANNER_H_
