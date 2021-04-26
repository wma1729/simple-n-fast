#include "clap.h"
#include "common.h"
#include <iostream>
#include <sstream>

namespace snf {

void
command::help() const
{
	std::cout << m_name << ": " << m_desc << std::endl;
	std::cout << std::endl;

	if (!m_options.empty()) {
		for (auto opt : m_options)
			std::cout << opt.name << '|' << opt.longname << '\t' << opt.desc << std::endl;
		std::cout << std::endl;
	}

	if (!m_subcmds.empty()) {
		for (auto sc : m_subcmds)
			std::cout << sc.m_name << '\t' << sc.m_desc << std::endl;
		std::cout << std::endl;
	}
}

bool
command::parse_args(int &idx, int argc, const char **argv)
{
	for (; idx < argc; idx++) {
		if (argv[idx][0] != '-') {
			break;
		}

		if (m_options.empty()) {
			help();
			return false;
		}

		for (cl_opt &opt : m_options) {
			if (streq(opt.name, argv[idx], true) || streq(opt.longname, argv[idx], true)) {
				bool arg_missing = true;

				switch (opt.type) {
					case cl_opt_type::t_bool:
						arg_missing = false;
						opt.arg = true;
						break;

					case cl_opt_type::t_int:
						if (argv[idx + 1]) {
							arg_missing = false;
							opt.arg = std::stoll(argv[idx + 1], 0, 0);
							idx++;
						}
						break;

					case cl_opt_type::t_real:
						if (argv[idx + 1]) {
							arg_missing = false;
							opt.arg = std::stod(argv[idx + 1], 0);
							idx++;
						}
						break;

					case cl_opt_type::t_string:
						if (argv[idx + 1]) {
							arg_missing = false;
							opt.arg = std::move(std::string {argv[idx + 1]});
							idx++;
						}
						break;

					case cl_opt_type::t_sequence:
						if (argv[idx + 1]) {
							std::stringstream ss(argv[idx + 1]);
							std::vector<std::string> svec;

							while (ss.good()) {
								std::string s;
								std::getline(ss, s, ',');
								if (!s.empty())
									svec.push_back(s);
							}

							arg_missing = false;
							opt.arg = std::move(svec);
							idx++;
						}
						break;

					default:
						arg_missing = false;
						break;
				}

				if (arg_missing) {
					std::cerr << "missing argument to option " << argv[idx] << std::endl;
					help();
					return false;
				}
			} else {
				help();
				return false;
			}
		}
	}

	for (const cl_opt & opt : m_options) {
		if (opt.required && !opt.found()) {
			std::cerr << "required option " << opt.name << " not specified" << std::endl;
			help();
			return false;
		}
	}

	return true;
}

bool
command::parse(int idx, int argc, const char **argv)
{
	if (idx + 1 == argc) {
		if (m_subcmds.empty() && m_options.empty()) {
			return true;
		} else {
			help();
			return false;
		}
	}

	idx++;
	m_found = true;

	if (argv[idx][0] == '-')
		if (!parse_args(idx, argc, argv))
			return false;

	if (!m_subcmds.empty()) {
		for (command & sc : m_subcmds) {
			if (streq(sc.m_name, argv[idx], true)) {
				return sc.parse(idx, argc, argv);
			}
		}
		help();
		return false;
	}

	return true;
}

} // namespace snf
