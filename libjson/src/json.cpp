#include "json.h"
#include "misc.h"
#include <sstream>
#include <stdexcept>
#include <iomanip>

namespace snf {
namespace json {

/*
 * Adds the key/value pair to the JSON object.
 * The key is first un-escaped.
 * @throw std::invalid_argument
 * @return reference to the current object.
 */
object &
object::add(const std::string &key, const value &val)
{
	insert(std::make_pair(string_unescape(key), val));
	return *this;
}

/*
 * Add the key/value pair to the JSON object.
 * Assumption is that the key/value are already un-escaped.
 * @return reference to the current object.
 */
object &
object::add(const value_type &kvpair)
{
	insert(kvpair);
	return *this;
}

/*
 * Does the object contains the key?
 * The key is un-escaped before the search.
 */
bool
object::contains(const std::string &key) const
{
	return (end() != find(string_unescape(key)));
}

/*
 * Gets the JSON value for the specified key.
 * If key is not found, an exception is thrown.
 * @throw std::out_of_range
 */
const value &
object::get(const std::string &key) const
{
	auto i = find(string_unescape(key));
	if (i == end()) {
		std::ostringstream oss;
		oss << "key " << key << " not found in JSON object!";
		throw std::out_of_range(oss.str());
	} else {
		return i->second;
	}
}

/*
 * Gets the JSON value for the specified key.
 * If key is not found, the default_value is returned.
 */
const value &
object::get(const std::string &key, const value &default_value) const
{
	auto i = find(string_unescape(key));
	if (i == end()) {
		return default_value;
	} else {
		return i->second;
	}
}

/*
 * Gets the string representation of the key/value pair
 * of the JSON object. Both the key and value (if the value
 * is indeed a string) are escaped.
 * @throw std::invalid_argument
 * @return the string representation of the JSON object member.
 */
std::string
object::str(const value_type &kvpair, bool pretty, int indent) const
{
	std::ostringstream oss;

	oss	<< "\""
		<< string_escape(kvpair.first)
		<< "\" : "
		<< kvpair.second.str(pretty, indent);

	return oss.str();
}

/*
 * Gets the string representation of the whole of the
 * JSON object. Keys and string values are escaped.
 * @throw std::invalid_argument
 * @return the string representation of the JSON object.
 */
std::string
object::str(bool pretty, int indent) const
{
	std::ostringstream oss;

	oss << "{";

	for (auto i = begin(); i != end(); ++i) {
		if (i != begin()) {
			oss << ",";
		}

		if (pretty)
			oss << std::endl << std::string(indent + 2, ' ');
		else
			oss << " ";

		oss << str(*i, pretty, indent + 2);
	}

	if (pretty)
		oss << std::endl << std::string(indent, ' ');
	else
		oss << " ";

	oss << "}";

	return oss.str();
}

/*
 * Adds a value to the JSON array.
 */
array &
array::add(const value &elem)
{
	emplace_back(elem);
	return *this;
}

/*
 * Gets the JSON value at the specified index.
 * If the index is invalid, an exception is thrown.
 * @throw std::out_of_range
 */
const value &
array::get(size_t index) const
{
	if (valid(index)) {
		return at(index);
	} else {
		std::ostringstream oss;
		oss << "index " << index << " is out of range in JSON array!";
		throw std::out_of_range(oss.str());
	}
}

/*
 * Gets the JSON value at the specified index.
 * If the index is invalid, the default_value is returned.
 */
const value &
array::get(size_t index, const value &default_value) const
{
	if (valid(index)) {
		return at(index);
	} else {
		return default_value;
	}
}

/*
 * Gets the string representation of the whole of the
 * JSON array. String values are escaped.
 * @throw std::invalid_argument
 * @return the string representation of the JSON array.
 */
std::string
array::str(bool pretty, int indent) const
{
	std::ostringstream oss;

	oss << "[";

	for (auto i = begin(); i != end(); ++i) {
		if (i != begin()) {
			oss << ",";
		}

		if (pretty)
			oss << std::endl << std::string(indent + 2, ' ');
		else
			oss << " ";

		oss << i->str(pretty, indent + 2);
	}

	if (pretty)
		oss << std::endl << std::string(indent, ' ');
	else
		oss << " ";

	oss << "]";

	return oss.str();
}

/* Cleanup the existing value */
void
value::clean(value &v)
{
	switch (v.m_type) {
	case T::T_STRING:
		delete v.m_val.s_val;
		break;

	case T::T_OBJECT:
		delete v.m_val.o_val;
		break;

	case T::T_ARRAY:
		delete v.m_val.a_val;
		break;

	default:
		break;
	}
	v.m_type = T::T_NULL;
	v.m_val.n_val = nullptr;
}

/* Copy the content of the specified value to this */
void
value::copy(const value &v)
{
	clean(*this);

	m_type = v.m_type;

	switch (v.m_type) {
	case T::T_BOOLEAN:
		m_val.b_val = v.m_val.b_val;
		break;

	case T::T_INTEGER:
		m_val.i_val = v.m_val.i_val;
		break;

	case T::T_REAL:
		m_val.d_val = v.m_val.d_val;
		break;

	case T::T_STRING:
		m_val.s_val = new std::string(*v.m_val.s_val);
		break;

	case T::T_OBJECT:
		m_val.o_val = new object(*v.m_val.o_val);
		break;

	case T::T_ARRAY:
		m_val.a_val = new array(*v.m_val.a_val);
		break;

	default:
		m_val.n_val = nullptr;
		break;
	}
}

/* Move the content of the specified value to this */
void
value::move(value &&v)
{
	clean(*this);

	m_type = v.m_type;

	switch (v.m_type) {
	case T::T_BOOLEAN:
		m_val.b_val = v.m_val.b_val;
		break;

	case T::T_INTEGER:
		m_val.i_val = v.m_val.i_val;
		break;

	case T::T_REAL:
		m_val.d_val = v.m_val.d_val;
		break;

	case T::T_STRING:
		m_val.s_val = v.m_val.s_val;
		v.m_val.s_val = nullptr;
		break;

	case T::T_OBJECT:
		m_val.o_val = v.m_val.o_val;
		v.m_val.o_val = nullptr;
		break;

	case T::T_ARRAY:
		m_val.a_val = v.m_val.a_val;
		v.m_val.o_val = nullptr;
		break;

	default:
		m_val.n_val = nullptr;
		break;
	}
}

/*
 * Assigns null to the JSON value.
 * @return the reference to the current value.
 */
const value &
value::operator= (std::nullptr_t np)
{
	clean(*this);
	return *this;
}

/*
 * Assigns object to the JSON value.
 * @return the reference to the current value.
 */
const value &
value::operator= (const object &o)
{
	clean(*this);
	m_type = T::T_OBJECT;
	m_val.o_val = new object(o);
	return *this;
}

/*
 * Assigns object to the JSON value (using move semantics).
 * @return the reference to the current value.
 */
const value &
value::operator= (object &&o)
{
	clean(*this);
	m_type = T::T_OBJECT;
	m_val.o_val = new object(std::move(o));
	return *this;
}

/*
 * Assigns array to the JSON value.
 * @return the reference to the current value.
 */
const value &
value::operator= (const array &a)
{
	clean(*this);
	m_type = T::T_ARRAY;
	m_val.a_val = new array(a);
	return *this;
}

/*
 * Assigns array to the JSON value (using move semantics).
 * @return the reference to the current value.
 */
const value &
value::operator= (array &&a)
{
	clean(*this);
	m_type = T::T_ARRAY;
	m_val.a_val = new array(std::move(a));
	return *this;
}

/*
 * Assigns specified value to the JSON value.
 * @return the reference to the current value.
 */
const value &
value::operator= (const value &v)
{
	if (this != &v)
		copy(v);
	return *this;
}

/*
 * Assigns specified value to the JSON value
 * (using move semantics).
 * @return the reference to the current value.
 */
const value &
value::operator= (value &&v)
{
	if (this != &v)
		move(std::move(v));
	return *this;
}

/*
 * Index operator for JSON object.
 */
value &
value::operator[] (const std::string &key)
{
	if (!is_object()) {
		clean(*this);
		m_type = T::T_OBJECT;
		m_val.o_val = new object();
	}
	return m_val.o_val->operator[](key);
}

/*
 * Index operator for JSON array.
 */
value &
value::operator[] (size_t index)
{
	if (!is_array()) {
		clean(*this);
		m_type = T::T_ARRAY;
		m_val.a_val = new array();
	}
	return m_val.a_val->operator[](index);
}

/*
 * Gets the boolean value. If the value is not a boolean,
 * an exception is thrown. is_boolean can be used to
 * determine if the value is a boolean.
 * @throw std::logic_error
 */
bool
value::get_boolean() const
{
	if (is_boolean()) {
		return m_val.b_val;
	} else {
		throw std::logic_error("invalid JSON value type!");
	}
}

/*
 * Gets the integer value. If the value is not an integer,
 * an exception is thrown. is_integer can be used to
 * determine if the value is an integer.
 * @throw std::logic_error
 */
int64_t
value::get_integer() const
{
	if (is_integer()) {
		return m_val.i_val;
	} else {
		throw std::logic_error("invalid JSON value type!");
	}
}

/*
 * Gets the real value. If the value is not a real number,
 * an exception is thrown. is_real can be used to
 * determine if the value is a real number.
 * @throw std::logic_error
 */
double
value::get_real() const
{
	if (is_real()) {
		return m_val.d_val;
	} else {
		throw std::logic_error("invalid JSON value type!");
	}
}

/*
 * Gets the string value. If the value is not a string,
 * an exception is thrown. is_string can be used to
 * determine if the value is a string. This is the only
 * method that returns the raw (un-escaped) string.
 * @throw std::logic_error
 */
const std::string &
value::get_string() const
{
	if (is_string()) {
		return *m_val.s_val;
	} else {
		throw std::logic_error("invalid JSON value type!");
	}
}

/*
 * Gets the object value. If the value is not an object,
 * an exception is thrown. is_object can be used to
 * determine if the value is an object.
 * @throw std::logic_error
 */
const object &
value::get_object() const
{
	if (is_object()) {
		return *m_val.o_val;
	} else {
		throw std::logic_error("invalid JSON value type!");
	}
}

/*
 * Gets the array value. If the value is not an array,
 * an exception is thrown. is_array can be used to
 * determine if the value is an array.
 * @throw std::logic_error
 */
const array &
value::get_array() const
{
	if (is_array()) {
		return *m_val.a_val;
	} else {
		throw std::logic_error("invalid JSON value type!");
	}
}

/*
 * Gets the string representation of the JSON value.
 * Keys and string values are escaped.
 * @throw std::invalid_argument
 * @return the string representation of the JSON object.
 */
std::string
value::str(bool pretty, int indent) const
{
	std::ostringstream oss;

	switch (m_type) {
	case T::T_NULL:
		oss << "null";
		break;

	case T::T_BOOLEAN:
		oss << std::boolalpha << m_val.b_val << std::noboolalpha;
		break;

	case T::T_INTEGER:
		oss << m_val.i_val;
		break;

	case T::T_REAL:
		oss << std::defaultfloat << m_val.d_val;
		break;

	case T::T_STRING:
		oss << "\"" << string_escape(*m_val.s_val) << "\"";
		break;

	case T::T_OBJECT:
		oss << m_val.o_val->str(pretty, indent);
		break;

	case T::T_ARRAY:
		oss << m_val.a_val->str(pretty, indent);
		break;
	}

	return oss.str();
}

} // json
} // snf
