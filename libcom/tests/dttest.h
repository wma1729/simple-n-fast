#include "timeutil.h"

class dttest : public snf::tf::test
{
private:
	static constexpr const char *class_name = "dttest";

public:
	dttest() : snf::tf::test() {}
	~dttest() {}

	virtual const char *name() const
	{
		return "DateTime";
	}

	virtual const char *description() const
	{
		return "Tests date time";
	}

	virtual bool execute(const snf::config *conf)
	{
		std::ostringstream oss;
		snf::datetime lt1 {};
		snf::datetime utc1 { true };

		oss << "date = " << lt1.str() << ", epoch = " << lt1.epoch(snf::unit::millisecond);
		TEST_LOG(oss.str()); oss.str("");

		oss << "date = " << utc1.str() << ", epoch = " << utc1.epoch(snf::unit::millisecond);
		TEST_LOG(oss.str()); oss.str("");

		snf::datetime lt2 { utc1.epoch(snf::unit::millisecond), snf::unit::millisecond, false };
		snf::datetime utc2 { utc1.epoch(snf::unit::millisecond), snf::unit::millisecond, true };

		oss << "date = " << lt2.str() << ", epoch = " << lt2.epoch(snf::unit::millisecond);
		TEST_LOG(oss.str()); oss.str("");

		oss << "date = " << utc2.str() << ", epoch = " << utc2.epoch(snf::unit::millisecond);
		TEST_LOG(oss.str()); oss.str("");

		ASSERT_EQ(const std::string &, lt1.str(), lt2.str(), "local time matches");
		ASSERT_EQ(const std::string &, utc1.str(), utc2.str(), "utc time matches");

		int64_t now = time(0);
		snf::datetime lt3 { now - 7200, snf::unit::second, false };
		oss << "date = " << lt3.str() << ", epoch = " << lt3.epoch(snf::unit::second);
		TEST_LOG(oss.str()); oss.str("");

		snf::datetime lt4 { now, snf::unit::second, false };
		oss << "date = " << lt4.str() << ", epoch = " << lt4.epoch(snf::unit::second);
		TEST_LOG(oss.str()); oss.str("");

		// Setting lt4 same as lt3
		lt4.set_year(lt3.get_year());
		lt4.set_month(lt3.get_month());
		lt4.set_day(lt3.get_day());
		lt4.set_hour(lt3.get_hour());
		lt4.set_minute(lt3.get_minute());
		lt4.set_second(lt3.get_second());
		lt4.set_day_of_week(lt3.get_day_of_week());
		lt4.recalculate();

		oss << "date = " << lt4.str() << ", epoch = " << lt4.epoch(snf::unit::second);
		TEST_LOG(oss.str()); oss.str("");

		ASSERT_EQ(int64_t, lt3.epoch(), lt4.epoch(), "time matches");

		now = time(0);
		snf::datetime utc3 { now - 86400, snf::unit::second, true };
		oss << "date = " << utc3.str() << ", epoch = " << utc3.epoch(snf::unit::second);
		TEST_LOG(oss.str()); oss.str("");

		snf::datetime utc4 { now, snf::unit::second, true };
		oss << "date = " << utc4.str() << ", epoch = " << utc4.epoch(snf::unit::second);
		TEST_LOG(oss.str()); oss.str("");

		// Setting utc4 same as utc3
		utc4.set_year(utc3.get_year());
		utc4.set_month(utc3.get_month());
		utc4.set_day(utc3.get_day());
		utc4.set_hour(utc3.get_hour());
		utc4.set_minute(utc3.get_minute());
		utc4.set_second(utc3.get_second());
		utc4.set_day_of_week(utc3.get_day_of_week());
		utc4.recalculate();

		oss << "date = " << utc4.str() << ", epoch = " << utc4.epoch(snf::unit::second);
		TEST_LOG(oss.str()); oss.str("");

		ASSERT_EQ(int64_t, utc3.epoch(), utc4.epoch(), "time matches");

		snf::datetime utc5 { now, snf::unit::second, true };
		oss << utc5.str(snf::time_format::imf);
		std::string datestr(oss.str()); oss.str("");
		TEST_LOG(datestr);

		snf::datetime utc6 = snf::datetime::get(datestr, snf::time_format::imf, true);
		ASSERT_EQ(int64_t, utc5.epoch(), utc6.epoch(), "time matches");

		return true;
	}
};
