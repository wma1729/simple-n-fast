#ifndef _SNF_AIEC_H_
#define _SNF_AIEC_H_

#include <system_error>
#include <netdb.h>

namespace snf {
namespace net {

enum class gai_error {
	try_again = EAI_AGAIN,
	bad_flags = EAI_BADFLAGS,
	non_recoverable_failure = EAI_FAIL,
	family_not_supported = EAI_FAMILY,
	out_of_memory = EAI_MEMORY,
	no_name = EAI_NONAME,
	service_not_supported = EAI_SERVICE,
	socktype_not_supported = EAI_SOCKTYPE
};

struct gai_cat : std::error_category
{
	const char *name() const noexcept override { return "gai"; }
	std::string message(int ev) const override { return gai_strerror(ev); }
};

const std::error_category &
gai_category() noexcept
{
	static gai_cat cat;
	return cat;
}

} // namespace net
} // namespace snf

namespace std
{
	template <>
	struct is_error_code_enum<snf::net::gai_error> : true_type {};

	std::error_code
	make_error_code(snf::net::gai_error e)
	{
		return std::error_code {
				static_cast<int>(e),
				snf::net::gai_category()
			};
	}

} // namespace std

#endif // _SNF_AIEC_H_
