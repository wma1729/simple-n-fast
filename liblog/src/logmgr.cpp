#include "logsev.h"
#include "logmgr.h"
#include "flogger.h"
#include "json.h"
#include <cstdarg>

namespace snf {
namespace log {

/**
 * Destroys the log manager object.
 */
manager::~manager()
{
	delete m_def_logger;

	std::lock_guard<std::mutex> g(m_lock);

	std::map<int, logger *>::const_iterator I;
	for (I = m_loggers.begin(); I != m_loggers.end(); ++I)
		delete I->second;
}

/**
 * Get severity from the json object.
 * @param obj [in] - the json object representing the logger conf
 * @return the severity if found, severity::all if not found.
 */
severity
manager::get_severity(const snf::json::object &obj)
{
	std::string s = std::move(obj.get_string("severity", "all"));
	return string_to_severity(s.c_str());
}

/**
 * Get the destination of the console logger from the
 * json object.
 * @param obj [in] - the json object representing the logger
 * @return the console logger destination.
 */
console_logger::destination
manager::get_destination(const snf::json::object &obj)
{
	std::string s = std::move(obj.get_string("destination", "var"));

	console_logger::destination dest = console_logger::destination::var;

	if (s == "stdout")
		dest = console_logger::destination::out;
	else if (s == "stderr")
		dest = console_logger::destination::err;

	return dest;
}

/**
 * Get the rotation scheme of the file logger from the
 * json object.
 * @param obj [in] - the json object representing the logger
 * @return the file logger rotation, nullptr in case of failure.
 */
rotation *
manager::get_rotation(const snf::json::object &obj)
{
	rotation *r = nullptr;

	if (obj.contains("rotation")) {
		const snf::json::value &v = obj.get("rotation");
		if (v.is_object()) {
			const snf::json::object &o = v.get_object();

			rotation::scheme scheme =
				rotation::string_to_scheme(o.get_string("scheme", "none"));
			int64_t size = o.get_integer("size", rotation::default_size);

			r = DBG_NEW rotation(scheme);
			r->size(size);
		}
	}

	return r;
}

/**
 * Get the retention scheme of the file logger from the
 * json object.
 * @param obj [in] - the json object representing the logger
 * @return the file logger retention, nullptr in case of failure.
 */
retention *
manager::get_retention(const snf::json::object &obj)
{
	retention *r = nullptr;

	if (obj.contains("retention")) {
		const snf::json::value &v = obj.get("retention");
		if (v.is_object()) {
			const snf::json::object &o = v.get_object();

			retention::scheme scheme =
				retention::string_to_scheme(o.get_string("scheme", "all"));
			int64_t argument = o.get_integer("argument", retention::default_argument);

			r = DBG_NEW retention(scheme, narrow_cast<int>(argument));
		}
	}

	return r;
}

/**
 * Create a logger from the json representation of the log
 * configuration.
 * @param obj [in] - the json object representing the logger
 * @return the logger created from the json object, nullptr if
 * the json does not contain the required key fields.
 */
logger *
manager::load(const snf::json::object &obj)
{
	if (!obj.contains("type"))
		return nullptr;

	const snf::json::value &vtype = obj.get("type");
	if (!vtype.is_string())
		return nullptr;

	logger *lgr = nullptr;

	const std::string &type = vtype.get_string();
	if (type == "console") {
		severity sev = get_severity(obj);
		std::string format =
			std::move(obj.get_string("format", console_logger::default_format));

		console_logger *clog = DBG_NEW console_logger(sev, format);
		clog->set_destination(get_destination(obj));

		lgr = clog;
	} else if (type == "file") {
		severity sev = get_severity(obj);
		std::string format =
			std::move(obj.get_string("format", file_logger::default_format));
		std::string path = std::move(obj.get_string("path", "."));

		file_logger *flog = DBG_NEW file_logger(path, sev, format);
		flog->make_path(obj.get_boolean("make_path", false));
		flog->sync(obj.get_boolean("sync", false));
		flog->set_name_format(
			obj.get_string("name_format", file_logger::default_name_format));

		rotation *rot = get_rotation(obj);
		if (rot) flog->set_rotation(rot);

		retention *ret = get_retention(obj);
		if (ret) flog->set_retention(ret);

		lgr = flog;
	}

	return lgr;
}

/**
 * Loads logger from the log configuration file for the
 * specified module.
 * @param logconf [in] - the log configuration file.
 * @param module  [in] - the module name to look for in the
 *                       configuration file.
 * @throw std::invalid_argument
 * @throw std::ios_base::failure
 * @throw snf::json::parsing_error
 */
void
manager::load(const std::string &logconf, const std::string &module)
{
	snf::json::value vobj = std::move(snf::json::from_file(logconf));
	if (!vobj.is_object())
		return;

	const snf::json::object &obj = vobj.get_object();
	if (!obj.contains(module))
		return;

	const snf::json::value &varr = obj.get(module);
	if (!varr.is_array())
		return;

	const snf::json::array &arr = varr.get_array();
	for (size_t i = 0; i < arr.size(); ++i) {
		const snf::json::value &val = arr.get(i);
		if (val.is_object()) {
			logger *lgr = load(val.get_object());
			if (lgr) {
				add_logger(lgr);
			}
		}
	}
}

/**
 * Adds a logger to the log manager.
 * @param l [in] - the logger to add.
 * @return an index that can be used later to
 *         remove the logger.
 */
int
manager::add_logger(logger *l)
{
	std::lock_guard<std::mutex> g(m_lock);
	int id = m_next_id++;
	m_loggers.insert(std::make_pair(id, l));
	return id;
}

/**
 * Removes a logger from the log manager.
 * @param id [in] - the logger index, returned by
 *                  add_logger, to remove.
 */
void
manager::remove_logger(int id)
{
	std::lock_guard<std::mutex> g(m_lock);
	m_loggers.erase(id);
}

/**
 * Logs the log record. The log record is sent to all the
 * registered loggers to log. If there is no logger
 * registered, it is sent to the default logger which is
 * a console logger.
 * @param rec [in] - the log record to log.
 */
void
manager::log(const record &rec)
{
	int logger_cnt = 0;

	{
		std::lock_guard<std::mutex> g(m_lock);
		std::map<int, logger *>::const_iterator I;
		for (I = m_loggers.begin(); I != m_loggers.end(); ++I) {
			logger_cnt++;
			I->second->log(rec);
		}
	}

	if (logger_cnt == 0) {
		if (m_def_logger == nullptr) {
			m_def_logger = DBG_NEW console_logger;
		}

		m_def_logger->log(rec);
	} else if (m_def_logger) {
		delete m_def_logger;
		m_def_logger = nullptr;
	}
}

/**
 * Log message.
 * @param sev   [in] - the log severity
 * @param cls   [in] - the class name
 * @param file  [in] - the file name
 * @param fcn   [in] - the function name
 * @param line  [in] - the line number
 * @param error [in] - the system error
 * @param fmt   [in] - the log text format
 */
void
manager::log(severity sev, const char *cls, const char *file, const char *fcn, int line,
	int error, const char *fmt, ...)
{
	record rec { cls, file, fcn, line, error, sev };

	if (fmt && *fmt) {
		char buf[BUFLEN + 1];
		va_list args;

		va_start(args, fmt);
		int n = vsnprintf(buf, BUFLEN, fmt, args);
		buf[n] = '\0';
		va_end(args);

		rec << buf;
	}

	rec << record::endl;
}

} // namespace log
} // namespace snf
