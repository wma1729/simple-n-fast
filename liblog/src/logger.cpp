#include "logger.h"

namespace snf {
namespace log {

/**
 * Logs the log record to the console.
 * @param rec [in] - the log record too log.
 */
void
console_logger::log(const record &rec)
{
	if (should_log(rec, get_severity())) {
		std::string line = std::move(rec.format(get_format()));
		if (get_destination() == destination::out)
			std::cout << line << std::endl;
		else if (get_destination() == destination::err)
			std::cerr << line << std::endl;
		else
			if (static_cast<int>(rec.get_severity()) > static_cast<int>(severity::info))
				std::cerr << line << std::endl;
			else 
				std::cout << line << std::endl;
	}
}

} // namespace log
} // namespace snf
