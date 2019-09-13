#include "transmit.h"
#include "error.h"

namespace snf {
namespace http {

/*
 * Sends the HTTP message data.
 *
 * @param [in] data      - data to send.
 * @param [in] datalen   - length of the data to be sent.
 * @param [in] exceptstr - exception message in case of error.
 *
 * @throws std::system_error in case of write error.
 *
 * @return E_ok on success, -ve error code on failure.
 */
int
transmitter::send_data(const void *data, size_t datalen, const std::string &exceptstr)
{
	int retval = E_ok;
	int to_write = static_cast<int>(datalen);
	int bwritten = 0;
	int syserr = 0;

	retval = m_io->write(data, to_write, &bwritten, 1000, &syserr);
	if (retval != E_ok) {
		throw std::system_error(
			syserr,
			std::system_category(),
			exceptstr);
	} else if (to_write != bwritten) {
		retval = E_write_failed;
	}

	return retval;
}

/*
 * Sends HTTP message body.
 *
 * @param [in] body - message body.
 *
 * @throws std::system_error in case of write errors.
 *
 * @return E_ok on success, -ve error code in case of failure.
 */
int
transmitter::send_body(body *body)
{
	int     retval = E_ok;
	size_t  chunklen;
	bool    body_chunked = body->chunked();
	int64_t body_length = body->length();

	while (body->has_next()) {
		chunklen = 0;
		const void *buf = body->next(chunklen);
	
		if (body_chunked) {
			std::ostringstream oss;
			oss << std::hex << chunklen << "\r\n";
			std::string s = std::move(oss.str());

			retval = send_data(
					s.data(),
					s.size(),
					"failed to send chunk size");
			if (retval != E_ok)
				break;
		}

		retval = send_data(
				buf,
				chunklen,
				body_chunked ? "failed to send chunk" : "failed to send body");
		if (retval != E_ok)
			break;

		if (!body_chunked)
			body_length -= chunklen;
	}

	if (retval == E_ok) {
		if (body_chunked) {
			retval = send_data(
					"0\r\n\r\n",
					5,
					"failed to send last chunk");
		} else if (body_length != 0) {
			retval = E_write_failed;
		}
	}

	return retval;
}

/*
 * Receives a line of HTTP message.
 *
 * @param [out] line      - message line.
 * @param [in]  exceptstr - exception message in case of error.
 *
 * @throws std::system_error in case of read error.
 *         snf::http::exception in case of invalid message line.
 *
 * @return E_ok on success, -ve error code in case of failure.
 */
int
transmitter::recv_line(std::string &line, const std::string &exceptstr)
{
	int retval = E_ok;
	int syserr = 0;

	retval = m_io->readline(line, 1000, &syserr);
	if (retval != E_ok)
		throw std::system_error(
			syserr,
			std::system_category(),
			exceptstr);

	size_t len = line.size();
	if ((len < 2) || (line[len - 1] != '\n') || (line[len - 2] != '\r'))
		throw bad_message("invalid request line/header");

	line.pop_back();
	line.pop_back();

	return retval;
}

/*
 * Sends HTTP request.
 *
 * @param [in] req - HTTP request.
 *
 * @throws std::system_error in case of write errors.
 *
 * @return E_ok on success, -ve error code in case of failure.
 */
int
transmitter::send_request(const request &req)
{
	std::ostringstream oss;

	oss << req;

	std::string s = std::move(oss.str());
	int retval = send_data(s.data(), s.size(), "failed to send request and headers");
	if (retval == E_ok) {
		body *req_body = req.get_body();
		if (req_body)
			retval = send_body(req_body);
	}

	return retval;
}

/*
 * Receives HTTP request.
 *
 * @throws std::system_error in case of read error.
 *         snf::http::exception in case of invalid message line.
 *
 * @return E_ok on success, -ve error code in case of failure.
 */
request
transmitter::recv_request()
{
	std::string req_line;
	headers     hdrs;

	recv_line(req_line, "unable to get request line");

	while (true) {
		std::string hdr_line;
		recv_line(hdr_line, "unable to get request header");

		if (hdr_line.empty())
			break;

		hdrs.add(hdr_line);
	}

	request_builder req_bldr;
	request req = std::move(req_bldr.request_line(req_line).with_headers(hdrs).build());

	body *b = nullptr;

	if (req.get_headers().is_message_chunked())
		b = body_factory::instance().from_socket_chunked(m_io);
	else if (req.get_headers().content_length() != 0)
		b = body_factory::instance().from_socket(m_io, req.get_headers().content_length());

	if (b != nullptr)
		req.set_body(b);

	return req;
}

/*
 * Sends HTTP responde.
 *
 * @param [in] resp - HTTP response.
 *
 * @throws std::system_error in case of write errors.
 *
 * @return E_ok on success, -ve error code in case of failure.
 */
int
transmitter::send_response(const response &resp)
{
	std::ostringstream oss;

	oss << resp;

	std::string s = std::move(oss.str());
	int retval = send_data(s.data(), s.size(), "failed to send response and headers");
	if (retval == E_ok) {
		body *resp_body = resp.get_body();
		if (resp_body)
			retval = send_body(resp_body);
	}

	return retval;
}

/*
 * Receives HTTP response.
 *
 * @throws std::system_error in case of read error.
 *         snf::http::exception in case of invalid message line.
 *
 * @return E_ok on success, -ve error code in case of failure.
 */
response
transmitter::recv_response()
{
	std::string resp_line;
	headers     hdrs;

	recv_line(resp_line, "unable to get response line");

	while (true) {
		std::string hdr_line;
		recv_line(hdr_line, "unable to get response header");

		if (hdr_line.empty())
			break;

		hdrs.add(hdr_line);
	}

	response_builder resp_bldr;
	response resp = std::move(resp_bldr.response_line(resp_line).with_headers(hdrs).build());

	body *b = nullptr;

	if (resp.get_headers().is_message_chunked())
		b = body_factory::instance().from_socket_chunked(m_io);
	else if (resp.get_headers().content_length() != 0)
		b = body_factory::instance().from_socket(m_io, resp.get_headers().content_length());

	if (b != nullptr)
		resp.set_body(b);

	return resp;
}

} // namespace http
} // namespace snf
