#ifndef _SNF_JSON_H_
#define _SNF_JSON_H_

#include <string>
#include <map>
#include <vector>
#include <initializer_list>
#include <istream>
#include <stdexcept>
#include "misc.h"

namespace snf {
namespace json {

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

class object
{
private:
	std::map<std::string, value> m_members;
	using member_t = typename std::map<std::string, value>::value_type;
	using const_iterator_t = typename std::map<std::string, value>::const_iterator;

	std::string str(const member_t &, bool, int) const;

public:
	object() = default;
	object(std::initializer_list<member_t>);
	object(const object &o) : m_members(o.m_members) {}
	object(object &&o) : m_members(std::move(o.m_members)) {}

	const object & operator=(const object &);
	object & operator=(object &&);
	value & operator[] (const std::string &);

	object & add(const std::string &, const value &);
	object & add(const member_t &);

	size_t size() const { return m_members.size(); }

	bool contains(const std::string &) const;
	const value & get(const std::string &) const;
	const value & get(const std::string &, const value &) const;

	const_iterator_t begin() const { return m_members.begin(); }
	const_iterator_t end() const { return m_members.end(); }

	std::string str(bool, int indent = 0) const;
};

class array
{
private:
	std::vector<value> m_elements;

public:
	array() = default;
	array(std::initializer_list<value>);
	array(const array &a) : m_elements(a.m_elements) {}
	array(array &&a) : m_elements(std::move(a.m_elements)) {}

	const array & operator=(const array &);
	array & operator=(array &&);
	value & operator[] (size_t);

	array & add(const value &);

	size_t size() const { return m_elements.size(); }
	bool valid(size_t index) const { return ((index >= 0) && (index < size())); }

	const value & get(size_t) const;
	const value & get(size_t, const value &) const;

	std::string str(bool, int indent = 0) const;
};

template<typename T1, typename T2 = void>
using EnableIfBoolean = typename std::enable_if<
			std::is_same<T1, bool>::value, T2
			>::type;

template<typename T1, typename T2 = void>
using EnableIfIntegral = typename std::enable_if<
			std::is_integral<T1>::value && !std::is_same<T1, bool>::value, T2
			>::type;

template<typename T1, typename T2 = void>
using EnableIfReal = typename std::enable_if<
			std::is_floating_point<T1>::value, T2
			>::type;

template<typename T1, typename T2 = void>
using EnableIfString = typename std::enable_if<
			std::is_convertible<T1, std::string>::value, T2
			>::type;

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
			std::string &&str = string_unescape(s);
			s_val = new std::string(str);
		}

		V(const std::string &s)
		{
			std::string &&str = string_unescape(s);
			s_val = new std::string(str);
		}

		V(std::string &&s)
		{
			std::string &&str = string_unescape(s);
			s_val = new std::string(std::move(str));
		}

		V(const object &o) : o_val(new object(o)) {}
		V(object &&o) : o_val(new object(std::move(o))) {}
		V(const array &a) : a_val(new array(a)) {}
		V(array &&a) : a_val(new array(std::move(a))) {}

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

	template<typename S>
	value(S && s, EnableIfString<S> * = 0)
		: m_type(T::T_STRING)
		, m_val(std::forward<S>(s)) {}

	value(object *o) : m_type(T::T_OBJECT), m_val(o) {}
	value(const object &o) : m_type(T::T_OBJECT), m_val(o) {}
	value(object &&o) : m_type(T::T_OBJECT), m_val(std::move(o)) {}
	value(array *a) : m_type(T::T_ARRAY), m_val(a) {}
	value(const array &a) : m_type(T::T_ARRAY), m_val(a) {}
	value(array &&a) : m_type(T::T_ARRAY), m_val(std::move(a)) {}
	value(const value &v) { copy(v); }
	value(value &&v) { move(std::move(v)); }
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
		std::string &&str = string_unescape(std::forward<S>(s));
		m_type = T::T_STRING;
		m_val.s_val = new std::string(std::move(str));
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
};

value from_string(const std::string &);
value from_file(const std::string &);
value from_stream(std::istream &);

} // json
} // snf

#endif // _SNF_JSON_H_
