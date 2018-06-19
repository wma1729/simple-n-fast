#include "logmgr.h"

class strm_log : public snf::tf::test
{
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
		ERROR_STRM(nullptr, "strm_log") << "this is the error log message"
			<< snf::log::record::endl;
		WARNING_STRM(nullptr, "strm_log") << "this is the warning log message"
			<< snf::log::record::endl;
		// This should not appear
		INFO_STRM(nullptr, "strm_log") << "this is the info log message"
			<< snf::log::record::endl;
		DEBUG_STRM(nullptr, "strm_log") << "this is the debug log message"
			<< snf::log::record::endl;
		TRACE_STRM(nullptr, "strm_log") << "this is the trace log message"
			<< snf::log::record::endl;
		return true;
	}
};
