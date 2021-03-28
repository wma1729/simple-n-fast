#include "flogger.h"
#include <regex>
#include <iostream>
#include "filesystem.h"
#include "dir.h"
#include "error.h"

namespace snf {
namespace log {

/**
 * Converts the string representation of rotation
 * scheme to its corresponding enum form.
 */
rotation::scheme
rotation::string_to_scheme(const std::string &str)
{
	rotation::scheme s = rotation::scheme::none;

	if (str.empty())
		return s;

	std::stringstream ss(str);

	while (ss.good()) {
		std::string substr;
		std::getline(ss, substr, '|');

		std::string tsubstr = std::move(snf::trim(substr));

		if (tsubstr == "daily") {
			s |= rotation::scheme::daily;
		} else if (tsubstr == "by_size") {
			s |= rotation::scheme::by_size;
		}
	}

	return s;
}

/**
 * Converts the string representation of retention
 * scheme to its corresponding enum form.
 */
retention::scheme
retention::string_to_scheme(const std::string &s)
{
	if (s.empty())
		return retention::scheme::all;

	if (s == "last_n_files")
		return retention::scheme::last_n_files;
	else if (s == "last_n_days")
		return retention::scheme::last_n_days;
	else
		return retention::scheme::all;
}

/**
 * Removes a log file.
 * @param path  [in] - the directory containing the file.
 * @param fname [in] - the log file name.
 */
void
retention::remove_file(const std::string &path, const std::string &fname)
{
	std::ostringstream oss;
	oss << path << snf::pathsep() << fname;
	std::string file = oss.str();
	snf::fs::remove_file(file.c_str());
}

/**
 * Purges the old log files.
 */
void
retention::purge()
{
	std::vector<file_attr> fa_vec;

	// read all the files matching the log file pattern
	for (auto &fa : directory(m_path, m_pattern))
		fa_vec.push_back(fa);

	// sort the file list based on time
	std::sort(fa_vec.begin(), fa_vec.end(),
		[](const file_attr &fa1, const file_attr &fa2) {
			return fa1.f_mtime > fa2.f_mtime;
		}
	);

	if (retain_by_file_count()) {
		int files_to_keep = m_argument; 

		for (auto &fa : fa_vec) {
			if (files_to_keep == 0) {
				remove_file(m_path, fa.f_name);
			} else {
				files_to_keep--;
			}
		}
	} else if (retain_by_days()) {
		int64_t now = epoch();
		int64_t older_time = now - (m_argument * 24 * 60 * 60);

		for (auto &fa : fa_vec) {
			if (fa.f_mtime < older_time) {
				remove_file(m_path, fa.f_name);
			}
		}
	}
}

/**
 * Generate the regular expression pattern for the log file name format.
 * @param capture    [in]  - generate regular expression pattern that can
 *                           capture date and/or sequence
 * @param date_index [out] - the capture index for date. -1 if %D for date
 *                           is not found.
 * @param seq_index  [out] - the capture index for sequence. -1 if %N for
 *                           sequence is not found.
 * @return the regular expression pattern.
 * @throws std::invalid_argument
 */
std::string
file_logger::regex_pattern(bool capture, int *date_index, int *seq_index)
{
	std::ostringstream oss;
	int index = 0;

	init_name_format();

	if (capture) {
		if (date_index == nullptr) {
			throw std::invalid_argument(
				"date index cannot be null when capture is requested");
		}
		if (seq_index == nullptr) {
			throw std::invalid_argument(
				"sequence index cannot be null when capture is requested");
		}
	}

	if (date_index) *date_index = -1;
	if (seq_index) *seq_index = -1;

	for (size_t i = 0; i < m_name_fmt.size(); ++i) {
		if (m_name_fmt[i] == '%') {
			++i;
			switch (m_name_fmt[i]) {
				case '%':
					oss << '%';
					break;

				case 'D':
					if (capture) {
						*date_index = ++index;
						oss << R"((\d{8}))";
					} else {
						oss << R"(\d{8})";
					}
					break;

				case 'N':
					if (capture) {
						*seq_index = ++index;
						oss << R"((\d{6}))";
					} else {
						oss << R"(\d{6})";
					}
					break;

				default:
					oss << '%' << m_name_fmt[i];
					break;
			}
		} else {
			oss << m_name_fmt[i];
		}
	}

	return oss.str();
}

/**
 * Parse the log file name and extract date and sequence number if any.
 * @param file_name [in]  - the file name to parse
 * @param date      [out] - the date string from the log file name
 * @param seq       [out] - the sequence string from the log file name
 */
void
file_logger::parse_name(const std::string &file_name, std::string &date, std::string &seq)
{
	int date_index;
	int seq_index;

	date.clear();
	seq.clear();

	std::string pat = std::move(regex_pattern(true, &date_index, &seq_index));

	if ((date_index == -1) && (seq_index == -1))
		return;

	std::smatch m;

	if (std::regex_search(file_name, m, std::regex { pat })) {
		if (date_index != -1)
			date = m[date_index].str();
		if (seq_index != -1)
			seq = m[seq_index].str();
	}
}

/**
 * Get the log file name.
 * @param lt [in] - the local time at the time of log file creation
 * @return the log file name.
 */
std::string
file_logger::name(const snf::datetime &lt)
{
	std::ostringstream oss;

	init_name_format();

	for (size_t i = 0; i < m_name_fmt.size(); ++i) {
		if (m_name_fmt[i] == '%') {
			++i;
			switch (m_name_fmt[i]) {
				case '%':
					oss << '%';
					break;

				case 'D':
					oss << std::setfill('0')
						<< std::setw(4) << lt.get_year()
						<< std::setw(2) << static_cast<int>(lt.get_month())
						<< std::setw(2) << lt.get_day();
					break;

				case 'N':
					oss << std::setfill('0')
						<< std::setw(6) << m_seqno;
					break;

				default:
					oss << '%' << m_name_fmt[i];
					break;
			}
		} else {
			oss << m_name_fmt[i];
		}
	}

	return oss.str();
}

/**
 * Get the log file name based on the last file read and the local time.
 * @param fa [in] - the file attributes of the last log file
 * @param lt [in] - the local time at the time of log file creation
 * @return the log file name
 */
std::string
file_logger::name(const snf::file_attr &fa, const snf::datetime &lt)
{
	if (fa.f_name.empty())
		return name(lt);

	// found a file; can we reuse it?

	std::string date_str, seq_str;
	parse_name(fa.f_name, date_str, seq_str);

	if (!date_str.empty()) {
		int year  = std::stoi(date_str.substr(0, 4));
		month mon = static_cast<month>(std::stoi(date_str.substr(4, 2)));
		int day   = std::stoi(date_str.substr(6, 2));

		if ((day != lt.get_day()) || (mon != lt.get_month()) || (year != lt.get_year())) {
			m_seqno = 0;
			return name(lt);
		}
	}

	if (!seq_str.empty()) {
		m_seqno = std::stoi(seq_str);
		if (rotate_by_size(fa.f_size)) {
			m_seqno++;
			return name(lt);
		}
	}

	m_size = fa.f_size;
	return fa.f_name;
}

/*
 * Open the log file.
 */
int
file_logger::open()
{
	std::ostringstream oss;
	oss << m_path << snf::pathsep() << m_name;

	m_file = DBG_NEW snf::file(oss.str(), 0022);

	snf::file::open_flags flags;
	flags.o_append = true;
	flags.o_create = true;
	flags.o_sync = m_sync;

	int retval = m_file->open(flags, 0600);
	if (E_ok != retval) {
		delete m_file;
		m_file = nullptr;
	}

	return retval;
}

/*
 * Close the log file.
 */
void 
file_logger::close()
{
	if (m_file) {
		m_file->close();
		delete m_file;
		m_file = nullptr;
		m_size = 0;
	}
}

/*
 * Open the log file. The old files are purged
 * before opening the log file.
 * @param lt [in] - the local time.
 */
void
file_logger::open(const snf::datetime &lt)
{
	snf::file_attr fa;
	int64_t mtime = -1;
	std::string pattern = std::move(regex_pattern());

	init_name_format();

	for (auto &ent : directory(m_path, pattern)) {
		if (ent.f_mtime > mtime) {
			fa = ent;
			mtime = ent.f_mtime;
		}
	}
	m_name = std::move(name(fa, lt));

	if (m_retention) {
		m_retention->set_path(m_path);
		m_retention->set_pattern(pattern);
		m_retention->purge();
	}

	if (E_ok == open())
		m_last_day = lt.get_day();
}

/*
 * Rotate the log file i.e close the already opened
 * log file and open the next log file. The old files
 * are purged before opening the log file.
 * @param lt [in] - the local time.
 */
void
file_logger::rotate(const snf::datetime &lt)
{
	bool rotate = false;

	if (rotate_daily(lt.get_day())) {
		// rotate
		rotate = true;
		m_seqno = 0;
	} else if (rotate_by_size(m_size)) {
		// rotate
		rotate = true;
		m_seqno++;
	}

	if (rotate) {
		close();
		m_name = std::move(name(lt));

		if (m_retention)
			m_retention->purge();

		if (E_ok == open())
			m_last_day = lt.get_day();
	}
}

/**
 * Logs the log record to the console.
 * @param rec [in] - the log record too log.
 */
void
file_logger::log(const record &rec)
{
	if (!should_log(rec, get_severity()))
		return;

	std::lock_guard<std::mutex> guard(m_file_lock);

	if (!m_file) {
		if (!snf::fs::exists(m_path.c_str())) {
			if (m_make_path) {
				if (E_ok != snf::fs::mkdir(m_path.c_str(), 0755)) {
					return;
				}
			} else {
				return;
			}
		}

		open(rec.get_timestamp());
	} else {
		rotate(rec.get_timestamp());
	}

	if (!m_file)
		return;

	std::string line = std::move(rec.format(get_format()));
	line.append(1, '\n');

	int bwritten;
	if (E_ok == m_file->write(line.c_str(), static_cast<int>(line.size()), &bwritten)) {
		m_size += line.size();
	}
}

/**
 * Reset the logger.
 */
void
file_logger::reset()
{
	std::lock_guard<std::mutex> guard(m_file_lock);
	close();
}

} // namespace log
} // namespace snf
