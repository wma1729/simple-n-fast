#include "flogger.h"
#include <regex>
#include <iostream>
#include "filesystem.h"
#include "dir.h"
#include "error.h"

namespace snf {
namespace log {

void
retention::remove_file(const std::string &path, const std::string &fname)
{
	std::ostringstream oss;
	oss << path << snf::pathsep() << fname;
	std::string file = oss.str();
	snf::fs::remove_file(file.c_str());
}

void
retention::purge()
{
	std::vector<file_attr> fa_vec;

	if (!read_directory(m_path, m_pattern, fa_vec))
		return;

	std::sort(fa_vec.begin(), fa_vec.end(),
		[](const file_attr &fa1, const file_attr &fa2) {
			return fa1.f_mtime > fa2.f_mtime;
		}
	);

	if (retain_by_file_count()) {
		int files_to_keep = m_args; 

		for (auto &fa : fa_vec) {
			if (files_to_keep == 0) {
				remove_file(m_path, fa.f_name);
			} else {
				files_to_keep--;
			}
		}
	} else if (retain_by_days()) {
		int64_t now = epoch();
		int64_t older_time = now - (m_args * 24 * 60 * 60);

		for (auto &fa : fa_vec) {
			if (fa.f_mtime < older_time) {
				remove_file(m_path, fa.f_name);
			}
		}
	}
}

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

std::string
file_logger::name(const snf::local_time &lt)
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
						<< std::setw(4) << lt.year()
						<< std::setw(2) << lt.month()
						<< std::setw(2) << lt.day();
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

std::string
file_logger::name(const snf::file_attr &fa, const snf::local_time &lt)
{
	if (fa.f_name.empty())
		return name(lt);

	// found a file; can we reuse it?

	std::string date_str, seq_str;
	parse_name(fa.f_name, date_str, seq_str);

	int year  = -1;
	int month = -1;
	int day   = -1;

	if (!date_str.empty()) {
		year  = std::stoi(date_str.substr(0, 4));
		month = std::stoi(date_str.substr(4, 2));
		day   = std::stoi(date_str.substr(6, 2));

		if ((day != lt.day()) || (month != lt.month()) || (year != lt.year())) {
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

int
file_logger::open()
{
	std::ostringstream oss;
	oss << m_path << snf::pathsep() << m_name;

	m_file = DBG_NEW snf::file(oss.str(), 0022);

	snf::file::open_flags flags;
	flags.o_append = true;
	flags.o_create = true;
	flags.o_sync = true;

	int retval = m_file->open(flags, 0600);
	if (E_ok != retval) {
		delete m_file;
		m_file = nullptr;
	}

	return retval;
}

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

void
file_logger::open(const snf::local_time &lt)
{
	snf::file_attr fa;
	std::string pattern = std::move(regex_pattern());

	init_name_format();

	read_newest(m_path, pattern, fa);
	m_name = std::move(name(fa, lt));

	if (m_retention) {
		m_retention->set_path(m_path);
		m_retention->set_pattern(pattern);
		m_retention->purge();
	}

	if (E_ok == open())
		m_last_day = lt.day();
}

void
file_logger::rotate(const snf::local_time &lt)
{
	bool rotate = false;

	if (rotate_daily(lt.day())) {
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
			m_last_day = lt.day();
	}
}

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

} // namespace log
} // namespace snf
