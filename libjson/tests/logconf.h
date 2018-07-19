#include "json.h"

class log_conf : public snf::tf::test
{
public:
	log_conf() : snf::tf::test() {}
	~log_conf() {}

	virtual const char *name() const
	{
		return "LogConfiguration";
	}

	virtual const char *description() const
	{
		return "Tests logging configuration data";
	}

	virtual bool execute(const snf::config *conf)
	{
		snf::json::value v_1 = OBJECT {
			KVPAIR("module1",
				ARRAY {
					OBJECT {
						KVPAIR("type", "console"),
						KVPAIR("severity", "INFO"),
						KVPAIR("format", "json-pretty"),
						KVPAIR("destination", "stdout")
					},
					OBJECT {
						KVPAIR("type", "file"),
						KVPAIR("severity", "TRACE"),
						KVPAIR("format", "json"),
						KVPAIR("path", "/tmp"),
						KVPAIR("makepath", false),
						KVPAIR("rotation", OBJECT {
							KVPAIR("scheme", "daily | by_size"),
							KVPAIR("size", 1000000)
						}),
						KVPAIR("retention", OBJECT {
							KVPAIR("scheme", "last_n_days"),
							KVPAIR("arg", 3)
						})
					}
				}
			),
			KVPAIR("module2",
				ARRAY {
					OBJECT {
						KVPAIR("type", "console"),
						KVPAIR("severity", "WARNING"),
						KVPAIR("format", "json-pretty"),
						KVPAIR("destination", "stderr")
					},
					OBJECT {
						KVPAIR("type", "file"),
						KVPAIR("severity", "DEBUG"),
						KVPAIR("format", "json"),
						KVPAIR("path", "/home/dumbo"),
						KVPAIR("makepath", true),
						KVPAIR("rotation", OBJECT {
							KVPAIR("scheme", "daily")
						}),
						KVPAIR("retention", OBJECT {
							KVPAIR("scheme", "last_n_files"),
							KVPAIR("arg", 15)
						})
					}
				}
			)
		};

		ASSERT_EQ(const std::string &,
			v_1["module1"][0]["type"].get_string(),
			"console",
			"type matches");
		ASSERT_EQ(const std::string &,
			v_1["module1"][0]["severity"].get_string(),
			"INFO",
			"severity matches");
		ASSERT_EQ(const std::string &,
			v_1["module1"][0]["format"].get_string(),
			"json-pretty",
			"format matches");
		ASSERT_EQ(const std::string &,
			v_1["module1"][0]["destination"].get_string(),
			"stdout",
			"destination matches");
		ASSERT_EQ(const std::string &,
			v_1["module1"][1]["type"].get_string(),
			"file",
			"type matches");
		ASSERT_EQ(const std::string &,
			v_1["module1"][1]["severity"].get_string(),
			"TRACE",
			"severity matches");
		ASSERT_EQ(const std::string &,
			v_1["module1"][1]["format"].get_string(),
			"json",
			"format matches");
		ASSERT_EQ(const std::string &,
			v_1["module1"][1]["path"].get_string(),
			"/tmp",
			"path matches");
		ASSERT_EQ(bool,
			v_1["module1"][1]["makepath"].get_boolean(),
			false,
			"makepath matches");
		ASSERT_EQ(const std::string &,
			v_1["module1"][1]["rotation"]["scheme"].get_string(),
			"daily | by_size",
			"rotation scheme matches");
		ASSERT_EQ(int64_t,
			v_1["module1"][1]["rotation"]["size"].get_integer(),
			1000000,
			"rotation size matches");
		ASSERT_EQ(const std::string &,
			v_1["module1"][1]["retention"]["scheme"].get_string(),
			"last_n_days",
			"retention scheme matches");
		ASSERT_EQ(int64_t,
			v_1["module1"][1]["retention"]["arg"].get_integer(),
			3,
			"retention argument matches");

		ASSERT_EQ(const std::string &,
			v_1["module2"][0]["type"].get_string(),
			"console",
			"type matches");
		ASSERT_EQ(const std::string &,
			v_1["module2"][0]["severity"].get_string(),
			"WARNING",
			"severity matches");
		ASSERT_EQ(const std::string &,
			v_1["module2"][0]["format"].get_string(),
			"json-pretty",
			"format matches");
		ASSERT_EQ(const std::string &,
			v_1["module2"][0]["destination"].get_string(),
			"stderr",
			"destination matches");
		ASSERT_EQ(const std::string &,
			v_1["module2"][1]["type"].get_string(),
			"file",
			"type matches");
		ASSERT_EQ(const std::string &,
			v_1["module2"][1]["severity"].get_string(),
			"DEBUG",
			"severity matches");
		ASSERT_EQ(const std::string &,
			v_1["module2"][1]["format"].get_string(),
			"json",
			"format matches");
		ASSERT_EQ(const std::string &,
			v_1["module2"][1]["path"].get_string(),
			"/home/dumbo",
			"path matches");
		ASSERT_EQ(bool,
			v_1["module2"][1]["makepath"].get_boolean(),
			true,
			"makepath matches");
		ASSERT_EQ(const std::string &,
			v_1["module2"][1]["rotation"]["scheme"].get_string(),
			"daily",
			"rotation scheme matches");
		ASSERT_EQ(const std::string &,
			v_1["module2"][1]["retention"]["scheme"].get_string(),
			"last_n_files",
			"retention scheme matches");
		ASSERT_EQ(int64_t,
			v_1["module2"][1]["retention"]["arg"].get_integer(),
			15,
			"retention argument matches");

		return true;
	}
};
