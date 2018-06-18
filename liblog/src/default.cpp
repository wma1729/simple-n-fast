#include "default.h"

namespace snf {
namespace log {

void
default_logger::log(const record &rec)
{
	if (should_log(rec, get_severity())) {
		std::string line = std::move(rec.format("%D %T %p.%t %s [%C] [%F.%c.%f.%l] %m"));
		if (static_cast<int>(rec.get_severity()) > static_cast<int>(severity::INFO))
			std::cerr << line << std::endl;
		else 
			std::cout << line << std::endl;
	}
}

} // namespace log
} // namespace snf
