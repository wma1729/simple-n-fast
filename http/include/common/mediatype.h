#ifndef _SNF_HTTP_MEDIA_TYPE_H_
#define _SNF_HTTP_MEDIA_TYPE_H_

#include <string>
#include <ostream>
#include <utility>
#include <vector>

namespace snf {
namespace http {

using param_vec_t = std::vector<std::pair<std::string, std::string>>;

static const std::string T_TEXT("text");
static const std::string T_APPLICATION("application");
static const std::string ST_PLAIN("plain");
static const std::string ST_JSON("json");

class media_type
{
private:
	std::string m_type;
	std::string m_subtype;
	param_vec_t m_parameters;

	void validate();

public:
	media_type() {}

	media_type(const std::string &str) { parse(str); }

	media_type(const std::string &t, const std::string &st)
		: m_type(t)
		, m_subtype(st)
	{
		validate();
	}

	media_type(const media_type &mt)
		: m_type(mt.m_type)
		, m_subtype(mt.m_subtype)
		, m_parameters(mt.m_parameters)
	{
		validate();
	}

	media_type(media_type &&mt)
		: m_type(std::move(mt.m_type))
		, m_subtype(std::move(mt.m_subtype))
		, m_parameters(std::move(mt.m_parameters))
	{
		validate();
	}

	virtual ~media_type() {}

	const media_type &operator=(const media_type &mt)
	{
		if (this != &mt) {
			m_type = mt.m_type;
			m_subtype = mt.m_subtype;
			m_parameters = mt.m_parameters;
		}
		return *this;
	}

	media_type &operator=(media_type &&mt)
	{
		if (this != &mt) {
			m_type = std::move(mt.m_type);
			m_subtype = std::move(mt.m_subtype);
			m_parameters = std::move(mt.m_parameters);
		}
		return *this;
	}

	const std::string type() const { return m_type; }
	const std::string subtype() const { return m_subtype; }
	const param_vec_t &param() const { return m_parameters; }

	void param(const std::pair<std::string, std::string> &kvpair)
	{
		m_parameters.push_back(kvpair);
	}

	void param(const std::string &k, const std::string &v)
	{
		param(std::make_pair(k, v));
	}

	void parse(const std::string &);
};

std::ostream & operator<< (std::ostream &, const media_type &);

} // namespace http
} // namespace snf

#endif // _SNF_HTTP_MEDIA_TYPE_H_
