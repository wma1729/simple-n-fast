#ifndef _SNF_CONFIG_H_
#define _SNF_CONFIG_H_

#include "common.h"
#include "error.h"
#include <map>

/**
 * Configuration options read from a file and store
 * in a map. The file has the format:
 * <pre>
 * key = value
 * </pre>
 * Empty lines are ignored. Comments starts with '#'.
 */
class Config
{
private:
	std::string configFile;
	std::map<std::string, std::string> kvMap;

public:
	/**
	 * Creates Config object withe the config file
	 * @param [in] cf - config file name.
	 */
	Config(const std::string &cf)
		: configFile(cf)
	{
	}

	/**
	 * Gets reference to the internal map. Do not try to
	 * manipulate it.
	 */
	const std::map<std::string, std::string> &getInternalMap() const
	{
		return kvMap;
	}

	int read();
	const char *getString(const std::string &, int *error = E_ok) const;
	int getInt(const std::string &, int *error = E_ok) const;
	int64_t getInt64(const std::string &, int *error = E_ok) const;
	bool getBool(const std::string &, int *error = E_ok) const;
	void dump() const;
};

#endif // _SNF_CONFIG_H_
