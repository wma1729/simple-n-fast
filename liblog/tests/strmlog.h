#include "logmgr.h"

class strm_log : public snf::tf::test
{
private:
	static constexpr const char *class_name = "strm_log";

public:
	strm_log() : snf::tf::test() {}
	~strm_log() {}

	virtual const char *name() const
	{
		return "StreamLogging";
	}

	virtual const char *description() const
	{
		return "Tests logging using stream intreface";
	}

	virtual bool execute(const snf::config *conf)
	{
		std::string text = "";

		ERROR_STRM(nullptr, class_name)
			<< "this is the " << 1 << "st log message"
			<< snf::log::record::endl;
		WARNING_STRM(nullptr, class_name)
			<< "this is the " << 2 << "nd log message"
			<< snf::log::record::endl;
		INFO_STRM(nullptr, class_name)
			<< "this is the " << 3 << "rd log message"
			<< snf::log::record::endl;
		DEBUG_STRM(nullptr, class_name)
			<< "this is the " << 4 << "th log message"
			<< snf::log::record::endl;
		TRACE_STRM(nullptr, class_name)
			<< "this is the " << 5 << "th log message"
			<< snf::log::record::endl;
		SYSERR_STRM(nullptr, class_name, 11)
			<< "this is the " << 6 << "th log message"
			<< snf::log::record::endl;

		LOG_SYSERR(nullptr, class_name, 12, "this is the %d%s log message", 7, "th");
		LOG_ERROR(nullptr, class_name, "this is the %d%s log message", 8, "th");
		LOG_WARNING(nullptr, class_name, "this is the %d%s log message", 9, "th");
		LOG_INFO(nullptr, class_name, "this is the %d%s log message", 10, "th");
		LOG_DEBUG(nullptr, class_name, "this is the %d%s log message", 11, "th");
		LOG_TRACE(nullptr, class_name, "this is the %d%s log message", 12, "th");

		return true;
	}
};
