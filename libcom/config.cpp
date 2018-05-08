#include "config.h"
#include <cstring>
#include <fstream>

/**
 * Reads the config file and populates the key/value pair map.
 */
void
Config::read()
{
	char buf[8192];
	std::string k;
	std::string v;
	std::ifstream ifs;

	ifs.exceptions(std::ios::failbit | std::ios::badbit | std::ios::eofbit);
	ifs.open(m_filename);

	while (ifs.getline(buf, sizeof(buf))) {
		buf[sizeof(buf) - 1] = '\0';
		size_t i = strlen(buf);

		// get rid of trailing spaces
		i = strlen(buf);
		while (i && isspace(buf[i - 1]))
			i--;
		buf[i] = '\0';

		// get rid of leading spaces
		char *ptr = buf;
		while (isspace(*ptr))
			ptr++;

		// ignore empty line or comments
		if ((*ptr == '\0') || (*ptr == '#'))
			continue;

		// get the key
		while (*ptr && (*ptr != '=') && !isspace(*ptr))
			k += *ptr;

		if (k.empty())
			continue;

		// ignore spaces between key and '=' sign
		if (isspace(*ptr)) {
			ptr++;
			while (isspace(*ptr))
				ptr++;

			if (*ptr && *ptr != '=')
				continue;
		}

		// skip past '=' sign
		if (*ptr == '=')
			ptr++;

		// ignore spaces between '=' sign and value
		while (isspace(*ptr))
			ptr++;

		if (*ptr == '\0')
			v = "true";
		else
			v = ptr;

		m_kvmap.insert(std::make_pair(k, v));
	}

	ifs.close();
}

/**
 * Gets the config value for the given key.
 *
 * @param [in]  key   - key name
 *
 * @return the config value in case of success, NULL if the
 * key is not found.
 */
const char *
Config::get(const std::string &key) const
{
	const char  *val = 0;

	std::map<std::string, std::string>::const_iterator I;
	I = m_kvmap.find(key);
	if (I != m_kvmap.end())
		val = I->second.c_str();

	return val;
}

/**
 * Gets the config value for the given key.
 *
 * @param [in]  key           - key name
 * @param [in]  default_value - default value if the key
 *                              is not found
 *
 * @return the config value in case of success, default_value if the
 * key is not found.
 */
const std::string &
Config::get(const std::string &key, const std::string &default_value) const
{
	std::map<std::string, std::string>::const_iterator I;
	I = m_kvmap.find(key);
	if (I != m_kvmap.end())
		return I->second;
	else
		return default_value;
}
