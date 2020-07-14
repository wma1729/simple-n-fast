#ifndef _SNF_HTTP_CMN_TRANSMIT_H_
#define _SNF_HTTP_CMN_TRANSMIT_H_

#include "nio.h"
#include "request.h"
#include "response.h"
#include "status.h"

namespace snf {
namespace http {

/*
 * Transmits HTTP request/response message.
 */
class transmitter
{
private:
	snf::net::nio   *m_io = nullptr;

	int send_data(const void *, size_t, const std::string &);
	int send_body(body *);

public:
	transmitter(snf::net::nio *io)
		: m_io(io)
	{
		if (m_io == nullptr)
			throw std::runtime_error("invalid input/output object specified");
	}

	transmitter(const transmitter &t) : m_io(t.m_io) {}

	transmitter(transmitter &&t) = delete;
	~transmitter() {}

	const transmitter &operator=(const transmitter &t)
	{
		if (this != &t)
			m_io = t.m_io;
		return *this;
	}

	transmitter &operator=(transmitter &&t) = delete;

	int send_request(const request &);
	request recv_request();
	int send_response(const response &);
	response recv_response();
};

response gen_error_resp(status_code, const std::string &);

} // namespace http
} // namespace snf

#endif // _SNF_HTTP_CMN_TRANSMIT_H_
