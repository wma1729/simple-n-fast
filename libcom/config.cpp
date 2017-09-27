#include <cstdio>
#include "common.h"
#include "config.h"
#include "util.h"

#define KLEN	512 - 1
#define VLEN	8192 - 512 - 1

/**
 * Reads the config file.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
Config::read()
{
	const char  *caller = "Config::read";
	int         retval = E_ok;
	int         lineno = 0;
	size_t      i;
	FILE        *fp = 0;
	char        buf[KLEN + VLEN + 210];   // 8400; large buffer
	char        key[KLEN + 1];	      // 512
	char        val[VLEN + 1];            // 7680
	char        *ptr = 0;

	fp = fopen(configFile.c_str(), "r");
	if (fp != 0) {
		while (fgets(buf, sizeof(buf), fp) != 0) {
			lineno++;

			// get rid of trailing spaces
			i = strlen(buf);
			while (i && isspace(buf[i - 1]))
				i--;
			buf[i] = '\0';

			// get rid of leading spaces
			ptr = buf;
			while (isspace(*ptr))
				ptr++;

			if ((*ptr == '\0') || (*ptr == '#') || ISNEWLINE(*ptr)) {
				// empty line or comment
				continue;
			}

			i = 0;
			while (i < KLEN) {
				if (isspace(*ptr) || (*ptr == '=') || (*ptr == '\0') || ISNEWLINE(*ptr)) {
					break;
				}
				key[i++] = *ptr++;
			}

			if (i == KLEN) {
				Log(WRN, caller, "key %s too large at line number %d", key, lineno);
				continue;
			}
			key[i] = '\0';

			if ((*ptr == '\0') || ISNEWLINE(*ptr)) {
				// no value; assume boolean with value of true
				kvMap[key] = "true";
				continue;
			}

			while (isspace(*ptr))
				ptr++;

			if (*ptr != '=') {
				Log(WRN, caller, "invalid pattern at line number %d", lineno);
				continue;
			}

			ptr++;
			while (isspace(*ptr))
				ptr++;

			i = 0;
			while (i < VLEN) {
				if ((*ptr == '\0') || ISNEWLINE(*ptr)) {
					break;
				}
				val[i++] = *ptr++;
			}

			if (i == VLEN) {
				Log(WRN, caller, "value %s too large at line number %d", val, lineno);
				continue;
			} else if (i == 0) {
				Log(WRN, caller, "no value specified for key %s at line number %d", key, lineno);
				continue;
			}

			val[i] = '\0';
			kvMap[key] = val;
		}

		if (ferror(fp)) {
			Log(ERR, caller, errno, "unable to read from %s.%d", configFile.c_str(), lineno);
			retval = E_read_failed;
		}

		fclose(fp);
	} else {
		Log(ERR, caller, errno, "unable to open %s", configFile.c_str());
		retval = E_open_failed;
	}

	return retval;
}

/**
 * Gets the config value as string.
 *
 * @param [in]  key   - key name
 * @param [out] error - error code in case of failure.
 *
 * @return the config value in case of success, NULL on failure.
 */
const char *
Config::getString(const std::string &key, int *error) const
{
	const char  *caller = "Config::getString";
	const char  *val = 0;

	if (error) *error = E_ok;

	std::map<std::string, std::string>::const_iterator I;
	I = kvMap.find(key);
	if (I == kvMap.end()) {
		Log(DBG, caller, "key %s not found", key.c_str());
		if (error) *error = E_not_found;
	} else {
		val = I->second.c_str();
		Log(DBG, caller, "%s = %s", key.c_str(), val);
	}

	return val;
}

/**
 * Gets the config value as integer.
 *
 * @param [in]  key   - key name
 * @param [out] error - error code in case of error.
 *
 * @return the config value in case of success, -1 on failure.
 * -1 could be a legitimate value for the key. To make sure,
 * check the error code as well.
 */
int
Config::getInt(const std::string &key, int *error) const
{
	const char  *caller = "Config::getInt";
	int         val = -1;

	if (error) *error = E_ok;

	std::map<std::string, std::string>::const_iterator I;
	I = kvMap.find(key);
	if (I == kvMap.end()) {
		Log(DBG, caller, "key %s not found", key.c_str());
		if (error) *error = E_not_found;
	} else {
		const char *vstr = I->second.c_str();
		Log(DBG, caller, "%s = %s", key.c_str(), vstr);
		val = atoi(vstr);
	}

	return val;
}

/**
 * Gets the config value as 64-bit integer.
 *
 * @param [in]  key   - key name
 * @param [out] error - error code in case of error.
 *
 * @return the config value in case of success, -1L on failure.
 * -1L could be a legitimate value for the key. To make sure,
 * check the error code as well.
 */
int64_t
Config::getInt64(const std::string &key, int *error) const
{
	const char  *caller = "Config::getInt64";
	int64_t     val = -1L;

	if (error) *error = E_ok;

	std::map<std::string, std::string>::const_iterator I;
	I = kvMap.find(key);
	if (I == kvMap.end()) {
		Log(DBG, caller, "key %s not found", key.c_str());
		if (error) *error = E_not_found;
	} else {
		const char *vstr = I->second.c_str();
		Log(DBG, caller, "%s = %s", key.c_str(), vstr);
		val = atoll(vstr);
	}

	return val;
}

/**
 * Gets the config value as boolean.
 *
 * @param [in]  key   - key name
 * @param [out] error - error code in case of error.
 *
 * @return the config value in case of success, false on failure.
 * false could be a legitimate value for the key. To make sure,
 * check the error code as well.
 */
bool
Config::getBool(const std::string &key, int *error) const
{
	const char  *caller = "Config::getBool";
	bool        val = false;

	if (error) *error = E_ok;

	std::map<std::string, std::string>::const_iterator I;
	I = kvMap.find(key);
	if (I == kvMap.end()) {
		Log(DBG, caller, "key %s not found", key.c_str());
		if (error) *error = E_not_found;
	} else {
		const char *vstr = I->second.c_str();
		Log(DBG, caller, "%s = %s", key.c_str(), vstr);

		if (strcmp(vstr, "1") == 0) {
			val = true;
		} else if (strcasecmp(vstr, "true") == 0) {
			val = true;
		} else if (strcasecmp(vstr, "yes") == 0) {
			val = true;
		} else if (strcasecmp(vstr, "y") == 0) {
			val = true;
		}
	}

	return val;
}

/**
 * Dumps all the entries from the config file.
 */
void
Config::dump() const
{
	const char *caller = "Config::dump";

	std::map<std::string, std::string>::const_iterator I;

	for (I = kvMap.begin(); I != kvMap.end(); ++I) {
		Log(DBG, caller, "%s = %s", I->first.c_str(), I->second.c_str());
	}
}
