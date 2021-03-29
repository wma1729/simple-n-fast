#include "clap.h"

class claptest : public snf::tf::test
{
private:
	static constexpr const char *class_name = "claptest";

public:
	claptest() : snf::tf::test() {}
	~claptest() {}

	virtual const char *name() const
	{
		return "CLAP";
	}

	virtual const char *description() const
	{
		return "Command-line argument processing";
	}

	virtual bool execute(const snf::config *conf)
	{
		const char *argv[] = {
			"testprogram",
			"init",
			"db",
			"-dh",
			"samplehost",
			"-dp",
			"123",
			"--dbname",
			"testdb",
			0
			};

		snf::command cmd{"testprogram", "a test program"};
		snf::command subcmd{"init", "initialize database" };
		snf::command subsubcmd{"db", "performe db initialization", std::vector<snf::cl_opt> {
			{"-dh", "--dbhost", "host name where database is running", true, snf::cl_opt_type::t_string },
			{"-dp", "--dbport", "port number where database is running", true, snf::cl_opt_type::t_int },
			{"-db", "--dbname", "database name where database is running", false, snf::cl_opt_type::t_string}
		}};
		subcmd.subcommand(subsubcmd);
		cmd.subcommand(subcmd);
		cl_parse(9, argv, cmd);

		const snf::command &c = cmd.subcommand();
		const snf::command &sc = c.subcommand();
		for (auto opt : sc.options()) {
			if (opt.found()) {
				if (opt.name == "-dh")
					std::cout << opt.name << " = " << std::get<std::string>(opt.arg) << std::endl;
				else if (opt.name == "-dp") 
					std::cout << opt.name << " = " << std::get<long long>(opt.arg) << std::endl;
				else if (opt.name == "-db")
					std::cout << opt.name << " = " << std::get<std::string>(opt.arg) << std::endl;
			}
		}

		return true;
	}
};
