#ifndef _SNF_CLAP_H_
#define _SNF_CLAP_H_

#include <string>
#include <vector>
#include <variant>
#include <map>
#include <stdexcept>

namespace snf {

enum class cl_opt_type
{
	t_none,         // No value assigned yet
	t_bool,         // A flag with no arguments
	t_int,          // An integer value
	t_real,         // A real value
	t_string,       // A string value
	t_sequence      // A comma-separated sequence of strings
};

using cl_opt_arg = std::variant<std::monostate, bool, long long, double, std::string, std::vector<std::string>>;

struct cl_opt
{
	std::string     name;           // short option name, like "-v"
	std::string     longname;       // long option name, like "--verbose"
	std::string     desc;           // option description
	bool            required;       // is the option required?
	cl_opt_type     type;           // option type
	cl_opt_arg      arg;            // option's argument

	cl_opt(const std::string &n, const std::string &ln, const std::string &d,
		bool r = false, cl_opt_type t = cl_opt_type::t_none)
		: name{n}, longname{ln}, desc{d}, required{r}, type{t} {}

	bool found() const { return arg.index() != 0; }
};

class command
{
private:
	std::string             m_name;
	std::string             m_desc;
	std::vector<cl_opt>     m_options;
	std::vector<command>    m_subcmds;
	bool                    m_found;

	friend bool cl_parse(int, const char **, command &);

	bool parse_args(int &, int, const char **);
	bool parse(int, int, const char **);

public:
	command(const std::string &name, const std::string &desc)
		: m_name{name}, m_desc{desc}, m_found{false} {} 
	command(const std::string &name, const std::string &desc, const std::vector<cl_opt> &opts)
		: m_name{name}, m_desc{desc}, m_options{opts}, m_found{false} {}
	command(const std::string &name, const std::string &desc, std::vector<cl_opt> &&opts)
		: m_name{name}, m_desc{desc}, m_options{std::move(opts)}, m_found{false} {}

	command(const command &) = default;
	command(command &&) = default;
	command & operator= (const command &) = default;
	command & operator= (command &&) = default;

	const std::string &name() const { return m_name; }

	const std::string &description() const { return m_desc; }
	void desciption(const std::string &desc) { m_desc = desc; }

	const std::vector<command> &subcommands() const { return m_subcmds; }
	const command &subcommand() const
	{
		for (const command &c : m_subcmds)
			if (c.found())
				return c;
		help();
		throw std::out_of_range("no sub command is specified");
	}

	void subcommand(const command &cmd) { m_subcmds.push_back(cmd); }
	void subcommands(const std::vector<command> &cmds) { for (auto cmd : cmds) m_subcmds.push_back(cmd); }
	void subcommands(std::vector<command> &&cmds) { m_subcmds = std::move(cmds); }

	const std::vector<cl_opt> &options() const { return m_options; }
	void options(const std::vector<cl_opt> &opts) { m_options = opts; }
	void options(std::vector<cl_opt> &&opts) { m_options = std::move(opts); }
	void option(const cl_opt &opt) { m_options.push_back(opt); }
	void option(cl_opt &&opt) { m_options.push_back(std::move(opt)); }

	bool found() const { return m_found; }
	void help() const;
};

inline bool
cl_parse(int argc, const char **argv, command &cmd)
{
	return cmd.parse(0, argc, argv);
}

} // namespace snf

#endif // _SNF_CLAP_H_
