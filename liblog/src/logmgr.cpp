#include "logsev.h"
#include "logmgr.h"
#include "logger.h"
#include "flogger.h"
#include <cstdarg>

namespace snf {
namespace log {

manager::~manager()
{
	delete m_def_logger;

	std::lock_guard<std::mutex> g(m_lock);

	std::map<int, logger *>::const_iterator I;
	for (I = m_loggers.begin(); I != m_loggers.end(); ++I)
		delete I->second;
}

void
manager::load(const snf::json::object &obj)
{
	if (!obj.contains("type"))
		return;

	const snf::json::value &vtype = obj.get("type");
	if (!vtype.is_string())
		return;

	const std::string &type = vtype.get_string();
	if (type == "console") {
		severity sev = severity::all;
		std::string format(console_logger::default_format);

		if (obj.contains("severity")) {
			const snf::json::value &v = obj.get("severity");
			if (v.is_string())
				sev = string_to_severity(v.get_string().c_str());
		}

		if (obj.contains("format")) {
			const snf::json::value &v = obj.get("format");
			if (v.is_string())
				format = v.get_string();
							
		}

		console_logger *logger = DBG_NEW console_logger(sev, format);

		if (obj.contains("destination")) {
			const snf::json::value &v = obj.get("destination");
			if (v.is_string()) {
				const std::string &dest = v.get_string();
				if (dest == "stdout")
					logger->set_destination(console_logger::destination::out);
				else if (dest == "stderr")
					logger->set_destination(console_logger::destination::err);
			}
		}

		add_logger(logger);
	} else if (type == "file") {
		severity sev = severity::all;
		std::string format(file_logger::default_format);
		std::string logpath(".");

		if (obj.contains("severity")) {
			const snf::json::value &v = obj.get("severity");
			if (v.is_string())
				sev = string_to_severity(v.get_string().c_str());
		}

		if (obj.contains("format")) {
			const snf::json::value &v = obj.get("format");
			if (v.is_string())
				format = v.get_string();
							
		}

		if (obj.contains("path")) {
			const snf::json::value &v = obj.get("path");
			if (v.is_string())
				logpath = v.get_string();
		}

		file_logger *logger = DBG_NEW file_logger(logpath, sev, format);

		if (obj.contains("makepath")) {
			const snf::json::value &v = obj.get("makepath");
			if (v.is_boolean())
				logger->make_path(v.get_boolean());
		}

		if (obj.contains("name_format")) {
			const snf::json::value &v = obj.get("name_format");
			if (v.is_string())
				logger->set_name_format(v.get_string());
		}

		if (obj.contains("rotation")) {
			rotation::scheme scheme = rotation::scheme::none;
			int64_t size = rotation::default_size;

			const snf::json::value &val = obj.get("rotation");
			if (val.is_object()) {
				const snf::json::object &o = val.get_object();
				if (o.contains("scheme")) {
					const snf::json::value &v = o.get("scheme");
					if (v.is_string())
						scheme = rotation::string_to_scheme(v.get_string());
				}

				if (o.contains("size")) {
					const snf::json::value &v = o.get("size");
					if (v.is_integer())
						size = v.get_integer();
				}

				rotation *r = DBG_NEW rotation(scheme);
				r->size(size);
				logger->set_rotation(r);
			}
		}

		if (obj.contains("retention")) {
			retention::scheme scheme = retention::scheme::all;
			int arg = retention::default_argument;

			const snf::json::value &val = obj.get("retention");
			if (val.is_object()) {
				const snf::json::object &o = val.get_object();
				if (o.contains("scheme")) {
					const snf::json::value &v = o.get("scheme");
					if (v.is_string())
						scheme = retention::string_to_scheme(v.get_string());
				}

				if (o.contains("argument")) {
					const snf::json::value &v = o.get("argument");
					if (v.is_integer())
						arg = narrow_cast<int>(v.get_integer());
				}

				retention *r = DBG_NEW retention(scheme, arg);
				logger->set_retention(r);
			}
		}
		add_logger(logger);
	}
}

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
		if (val.is_object())
			load(val.get_object());
	}
}

int
manager::add_logger(logger *l)
{
	std::lock_guard<std::mutex> g(m_lock);
	int id = m_next_id++;
	m_loggers.insert(std::make_pair(id, l));
	return id;
}

void
manager::remove_logger(int id)
{
	std::lock_guard<std::mutex> g(m_lock);
	m_loggers.erase(id);
}

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
