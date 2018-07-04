#ifndef _SNF_JSON_H_
#define _SNF_JSON_H_

#include <string>
#include <map>
#include <vector>
#include <initializer_list>
#include <istream>
#include <stdexcept>
#include "dbg.h"

namespace snf {
namespace json {

/*
 * Unescapes the input string i.e. all the escape
 * characters including the utf espace characters
 * of the form \uxxxx, where x is the hex digit are
 * un-escaped to their raw form.
 * @throw - std::invalid_argument
 * @return un-escaped (raw) string.
 */
std::string string_unescape(const std::string &);

/*
 * Escapes the input string i.e. all the special
 * characters including multi-byte utf characters
 * are escaped so they are printable/transferable.
 * @throw - std::invalid_argument
 * @return escaped string.
 */
std::string string_escape(const std::string &);

/*
 * Notes on string storage:
 * All the strings are stored in their raw form in
 * memory. Only while dumping i.e. using << operator
 * or str() method of the value, or the object, or
 * the array class, the strings are escaped. To get
 * the raw form, use get_string() method of the
 * value class.
 */

/*
 * Exception thrown when a parsing error is
 * encountered. Where possible, the corresponding
 * row and column number are encapsulated in the
 * exception.
 */
class parsing_error : public std::runtime_error
{
private:
	int m_row;
	int m_col;

public:
	parsing_error(const std::string &msg, int row = -1, int col = -1)
		: std::runtime_error(msg)
		, m_row(row)
		, m_col(col)
	{
	}

	parsing_error(const char *msg, int row = -1, int col = -1)
		: std::runtime_error(msg)
		, m_row(row)
		, m_col(col)
	{
	}

	int row() const { return m_row; }
	int col() const { return m_col; }
};

class value;

/*
 * JSON object. Implemented as a C++ map. All std::map
 * operations are available to use on the JSON object.
 * A few additional methods are provided for convenience.
 */
class object : public std::map<std::string, value>
{
private:
	using value_type = std::map<std::string, value>::value_type;
	std::string str(const value_type &, bool, int) const;

public:
	using std::map<std::string, value>::map;

	object & add(const std::string &, const value &);
	object & add(const value_type &);

	bool contains(const std::string &) const;

	const value & get(const std::string &) const;
	const value & get(const std::string &, const value &) const;

	std::string str(bool, int indent = 0) const;

	friend std::ostream & operator<<(std::ostream & os, const object & o)
	{
		os << o.str(false);
		return os;
	}
};

/*
 * JSON array. Implemented as a C++ vector. All std::vector
 * operations are available to use on the JSON array.
 * A few additional methods are provided for convenience.
 */
class array : public std::vector<value>
{
public:
	using std::vector<value>::vector;

	array & add(const value &);

	bool valid(size_t index) const { return ((index >= 0) && (index < size())); }

	const value & get(size_t) const;
	const value & get(size_t, const value &) const;

	std::string str(bool, int indent = 0) const;

	friend std::ostream & operator<<(std::ostream & os, const array & a)
	{
		os << a.str(false);
		return os;
	}
};

/* Enable the template for boolean types. */
template<typename T1, typename T2 = void>
using EnableIfBoolean = typename std::enable_if<
			std::is_same<T1, bool>::value, T2
			>::type;

/* Enable the template for integral types, excluding booleans. */
template<typename T1, typename T2 = void>
using EnableIfIntegral = typename std::enable_if<
			std::is_integral<T1>::value && !std::is_same<T1, bool>::value, T2
			>::type;

/* Enable the template for real types. */
template<typename T1, typename T2 = void>
using EnableIfReal = typename std::enable_if<
			std::is_floating_point<T1>::value, T2
			>::type;

/* Enable the template for string types. */
template<typename T1, typename T2 = void>
using EnableIfString = typename std::enable_if<
			std::is_convertible<T1, std::string>::value, T2
			>::type;

/*
 * JSON value. It can hold
 * - null
 * - boolean
 * - integer (all integers are stored as 64-bit signed integers)
 * - real (all real values are stored as double)
 * - string
 * - JSON object
 * - JSON array
 */
class value
{
private:
	enum class T
	{
		T_NULL,
		T_BOOLEAN,
		T_INTEGER,
		T_REAL,
		T_STRING,
		T_OBJECT,
		T_ARRAY
	} m_type;

	union V
	{
		V() : n_val(nullptr) {}
		V(bool b) : b_val(b) {}
		V(int64_t i) : i_val(i) {}
		V(double d) : d_val(d) {}

		V(const char *s)
		{
			s_val = DBG_NEW std::string(string_unescape(s));
		}

		V(const std::string &s)
		{
			s_val = DBG_NEW std::string(string_unescape(s));
		}

		V(std::string &&s)
		{
			s_val = DBG_NEW std::string(std::move(string_unescape(s)));
		}

		V(const object &o) : o_val(DBG_NEW object(o)) {}
		V(object &&o) : o_val(DBG_NEW object(std::move(o))) {}
		V(const array &a) : a_val(DBG_NEW array(a)) {}
		V(array &&a) : a_val(DBG_NEW array(std::move(a))) {}

		std::nullptr_t n_val;
		bool b_val;
		int64_t i_val;
		double d_val;
		std::string *s_val;
		object *o_val;
		array *a_val;
	} m_val;

	void clean(value &);
	void copy(const value &);
	void move(value &&);

public:
	value() : m_type(T::T_NULL), m_val() {}

	template<typename B>
	value(B b, EnableIfBoolean<B> * = 0)
		: m_type(T::T_BOOLEAN)
		, m_val(b) {}

	template<typename I>
	value(I i, EnableIfIntegral<I> * = 0)
		: m_type(T::T_INTEGER)
		, m_val(static_cast<int64_t>(i)) {}

	template<typename R>
	value(R d, EnableIfReal<R> * = 0)
		: m_type(T::T_REAL)
		, m_val(static_cast<double>(d)) {}

	value(const char *s)
	{
		if (s == nullptr) {
			m_type = T::T_NULL;
		} else {
			m_type = T::T_STRING;
			m_val.s_val = DBG_NEW std::string(string_unescape(s));
		}
	}

	value(const std::string &s) : m_type(T::T_STRING), m_val(s) {}
	value(std::string &&s) : m_type(T::T_STRING), m_val(std::move(s)) {}
	value(const object &o) : m_type(T::T_OBJECT), m_val(o) {}
	value(object &&o) : m_type(T::T_OBJECT), m_val(std::move(o)) {}
	value(const array &a) : m_type(T::T_ARRAY), m_val(a) {}
	value(array &&a) : m_type(T::T_ARRAY), m_val(std::move(a)) {}
	value(const value &v) : m_type(v.m_type) { copy(v); }
	value(value &&v) : m_type(v.m_type) { move(std::move(v)); }
	~value() { clean(*this); }

	const value & operator= (std::nullptr_t);

	template<typename B>
	EnableIfBoolean<B, const value &> operator= (B b)
	{
		clean(*this);
		m_type = T::T_BOOLEAN;
		m_val.b_val = b;
		return *this;
	}

	template<typename I>
	EnableIfIntegral<I, const value &> operator= (I i)
	{
		clean(*this);
		m_type = T::T_INTEGER;
		m_val.i_val = static_cast<int64_t>(i);
		return *this;
	}

	template<typename R>
	EnableIfReal<R, const value &> operator= (R r)
	{
		clean(*this);
		m_type = T::T_REAL;
		m_val.d_val = static_cast<double>(r);
		return *this;
	}

	template<typename S>
	EnableIfString<S, const value &> operator= (S && s)
	{
		clean(*this);
		m_type = T::T_STRING;
		m_val.s_val = DBG_NEW std::string(std::move(string_unescape(std::forward<S>(s))));
		return *this;
	}

	const value & operator= (const object &);
	const value & operator= (object &&);
	const value & operator= (const array &);
	const value & operator= (array &&);
	const value & operator= (const value &);
	const value & operator= (value &&);
	value & operator[] (const std::string &);
	value & operator[] (size_t);

	bool is_null()    const { return (m_type == T::T_NULL); }
	bool is_boolean() const { return (m_type == T::T_BOOLEAN); }
	bool is_integer() const { return (m_type == T::T_INTEGER); }
	bool is_real()    const { return (m_type == T::T_REAL); }
	bool is_string()  const { return (m_type == T::T_STRING); }
	bool is_object()  const { return (m_type == T::T_OBJECT); }
	bool is_array()   const { return (m_type == T::T_ARRAY); }

	bool get_boolean() const;
	int64_t get_integer() const;
	double get_real() const;
	const std::string &get_string() const;
	const object &get_object() const;
	const array &get_array() const;

	std::string str(bool, int indent = 0) const;

	friend std::ostream & operator<<(std::ostream & os, const value & v)
	{
		os << v.str(false);
		return os;
	}
};

value from_string(const std::string &);
value from_file(const std::string &);
value from_stream(std::istream &);

#define KVPAIR       std::make_pair
#define OBJECT       snf::json::object
#define ARRAY        snf::json::array

} // json
} // snf

#endif // _SNF_JSON_H_
