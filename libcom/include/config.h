#ifndef _SNF_CONFIG_H_
#define _SNF_CONFIG_H_

#include <string>
#include <map>

namespace snf {

/**
 * Configuration options read from a file and store
 * in a map. The file has the format:
 * <pre>
 * key = value
 * </pre>
 * Empty lines are ignored. Comments starts with '#'.
 */
class config
{
private:
	std::string m_filename;
	std::map<std::string, std::string> m_kvmap;
	using const_iterator_t = std::map<std::string, std::string>::const_iterator;

	void read();
public:
	/**
	 * Creates config object withe the config file.
	 * @param [in] cf - config file name.
	 */
	config(const std::string &cf)
		: m_filename(cf)
	{
		read();
	}

	/**
	 * Gets the config file name.
	 * @return name of the config file.
	 */
	const std::string &name() const { return m_filename; }

	const_iterator_t begin() const { return m_kvmap.begin(); }
	const_iterator_t end() const { return m_kvmap.end(); }

	const char *get(const std::string &) const;
	const std::string &get(const std::string &, const std::string &) const;
};

} // namespace snf

#endif // _SNF_CONFIG_H_
