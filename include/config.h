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

	int read();
	const char *getString(const std::string &, int *error = E_ok);
	int getInt(const std::string &, int *error = E_ok);
	int64_t getInt64(const std::string &, int *error = E_ok);
	bool getBool(const std::string &, int *error = E_ok);
	void dump();
};

#endif // _SNF_CONFIG_H_
