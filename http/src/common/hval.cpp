#include "hval.h"
#include "status.h"

namespace snf {
namespace http {

void
parse(size_t &number, const std::string &istr)
{
	for (size_t i = 0; i < istr.length(); ++i) {
		if (!isdigit(istr[i])) {
			std::ostringstream oss;
			oss << "incorrect number (" << istr << ") specified";
			throw bad_message(oss.str());
		}
	}

	number = std::stoull(istr);
}

void
parse(std::string &ostr, const std::string &istr)
{
	ostr = istr;
}

void
parse(std::vector<std::string> &strvec, const std::string &istr)
{
	bool next = !istr.empty();
	std::ostringstream oss;
	scanner scn{istr};

	while (next) {
		std::string s;

		if (!scn.read_token(s)) {
			oss << "no token found in (" << istr << ")";
			throw bad_message(oss.str());
		}

		strvec.push_back(std::move(s));

		scn.read_opt_space();

		if (scn.read_special(','))
			scn.read_opt_space();
		else
			next = false;
	}
}

void
parse(std::vector<token> &tokens, const std::string &istr)
{
	bool next = !istr.empty();
	std::ostringstream oss;
	scanner scn{istr};

	while (next) {
		token t;

		if (!scn.read_token(t.name)) {
			oss << "no token found in (" << istr << ")";
			throw bad_message(oss.str());
		}

		scn.read_opt_space();

		if (scn.read_special(';')) {
			if (!scn.read_parameters(t.parameters))
				throw bad_message("no parameters found");
		}

		tokens.push_back(std::move(t));

		scn.read_opt_space();

		if (scn.read_special(','))
			scn.read_opt_space();
		else
			next = false;
	}
}

void
parse(uri &url, const std::string &uristr)
{
	try {
		uri the_url{uristr};
		url = std::move(the_url);
	} catch (snf::http::bad_uri &ex) {
		throw bad_message(ex.what());
	}
}

void
parse(host_port &hp, const std::string &hoststr)
{
	try {
		std::ostringstream oss;
		oss << "http://" << hoststr;
		std::string uristr = oss.str();

		snf::http::uri the_uri(uristr);

		if (the_uri.get_host().is_present()) {
			hp.host = the_uri.get_host().get();
		} else {
			oss.str("");
			oss << "invalid host string specified: " << hoststr;
			throw bad_message(oss.str());
		}

		if (the_uri.get_port().is_present()) {
			hp.port = the_uri.get_port().numeric_port();
		}
	} catch (snf::http::bad_uri &ex) {
		throw bad_message(ex.what());
	}
}

void
parse(media_type &mt, const std::string &istr)
{
	scanner scn{istr};

	if (!scn.read_token(mt.type))
		throw bad_message("no media type found");

	if (!scn.read_special('/'))
		throw bad_message("media type is not followed by \'/\'");

	if (!scn.read_token(mt.subtype))
		throw bad_message("no media subtype found");

	scn.read_opt_space();

	if (scn.read_special(';')) {
		scn.read_opt_space();

		if (!scn.read_parameters(mt.parameters))
			throw bad_message("no parameters found");
	}
}

void
parse(std::vector<via> &viavec, const std::string &istr)
{
	bool next = !istr.empty();
	std::ostringstream oss;
	scanner scn{istr};

	while (next) {
		via v;
		std::string s;

		scn.read_opt_space();

		if (!scn.read_token(v.proto, false))
			throw bad_message("no protocol/version found");

		if (scn.read_special('/')) {
			if (!scn.read_token(s))
				throw bad_message("no version found");
		} else {
			s = v.proto;
			v.proto.clear();
		}

		v.ver = version{s, true};

		s.clear();

		if (!scn.read_space()) {
			oss << "no space after (";
			if (v.proto.empty())
				oss << v.ver;
			else
				oss << v.proto << "/" << v.ver;
			oss << ")";
			throw bad_message(oss.str());
		}

		if (!scn.read_uri(s))
			throw bad_message("no URI found");
	
		oss.str("");
		oss << "http://" << s;
		uri the_uri{oss.str()};

		v.url = std::move(the_uri);

		scn.read_space();

		scn.read_comments(v.comments);

		viavec.push_back(v);

		scn.read_opt_space();

		if (scn.read_special(','))
			scn.read_opt_space();
		else
			next = false;
	}
}

void
parse(snf::datetime &dt , const std::string &istr)
{
	try {
		dt = snf::datetime::get(istr, snf::time_format::imf, true);
	} catch (const std::invalid_argument &ex) {
		throw bad_message(ex.what());
	}
}

void
valid_connection(const std::string &cnxn)
{
	if ((cnxn == CONNECTION_CLOSE) ||
		(cnxn == CONNECTION_KEEP_ALIVE) ||
		(cnxn == CONNECTION_UPGRADE)) {
		return;
	}

	std::ostringstream oss;
	oss << "connection option " << cnxn << " is not implemented";
	throw not_implemented(oss.str());
}

void
valid_media_type(const media_type &mt)
{
	std::ostringstream oss;
		
	if (mt.type == CONTENT_TYPE_T_TEXT) {
		if (mt.subtype != CONTENT_TYPE_ST_PLAIN) {
			oss << "subtype " << mt.subtype
				<< " is not implemented for type " << mt.type;
			throw not_implemented(oss.str());
		}
	} else if (mt.type == CONTENT_TYPE_T_APPLICATION) {
		if (mt.subtype != CONTENT_TYPE_ST_JSON) {
			oss << "subtype " << mt.subtype
				<< " is not implemented for type " << mt.type;
			throw not_implemented(oss.str());
		}
	} else {
		oss << "type " << mt.type << " is not implemented";
		throw not_implemented(oss.str());
	}
}

void
valid_encoding(const std::string &coding)
{
	if ((coding == CONTENT_ENCODING_COMPRESS) ||
		(coding == CONTENT_ENCODING_X_COMPRESS) ||
		(coding == CONTENT_ENCODING_GZIP) ||
		(coding == CONTENT_ENCODING_X_GZIP) ||
		(coding == CONTENT_ENCODING_DEFLATE)) {
		return;
	}

	std::ostringstream oss;
	oss << "content-encoding " << coding << " is not implemented";
	throw not_implemented(oss.str());
}

} // namespace http
} // namespace snf
