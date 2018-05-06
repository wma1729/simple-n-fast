#ifndef _SNF_JSON_H_
#define _SNF_JSON_H_

#if defined(_WIN32)
	#if defined(_SNF_EXPORTING_)
		#define SNF_DLL_DECLSPEC    __declspec(dllexport)
	#else // !_SNF_EXPORTING_
		#define SNF_DLL_DECLSPEC    __declspec(dllimport)
	#endif
#else // !_WIN32
	#define SNF_DLL_DECLSPEC
#endif

#include <string>
#include <map>
#include <vector>
#include <initializer_list>
#include <istream>

namespace snf {
namespace json {

class SNF_DLL_DECLSPEC value;

class SNF_DLL_DECLSPEC object
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

class SNF_DLL_DECLSPEC array
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

class SNF_DLL_DECLSPEC value
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
		V();
		V(bool);
		V(int64_t);
		V(double);
		V(const char *);
		V(const std::string &);
		V(std::string &&);
		V(const object &);
		V(object &&);
		V(const array &);
		V(array &&);

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
	value(bool b) : m_type(T::T_BOOLEAN), m_val(b) {}
	value(int64_t i) : m_type(T::T_INTEGER), m_val(i) {}
	value(double d) : m_type(T::T_REAL), m_val(d) {}
	value(const std::string &s) : m_type(T::T_STRING), m_val(s) {}
	value(std::string &&s) : m_type(T::T_STRING), m_val(std::move(s)) {}
	value(object *o) : m_type(T::T_OBJECT), m_val(o) {}
	value(const object &o) : m_type(T::T_OBJECT), m_val(o) {}
	value(object &&o) : m_type(T::T_OBJECT), m_val(std::move(o)) {}
	value(array *a) : m_type(T::T_ARRAY), m_val(a) {}
	value(const array &a) : m_type(T::T_OBJECT), m_val(a) {}
	value(array &&a) : m_type(T::T_OBJECT), m_val(std::move(a)) {}
	value(const value &v) { copy(v); }
	value(value &&v) { move(std::move(v)); }
	~value() { clean(*this); }

	const value & operator= (bool);
	const value & operator= (int64_t);
	const value & operator= (double);
	const value & operator= (const std::string &);
	const value & operator= (std::string &&);
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

	void reset() { clean(*this); }

	std::string str(bool, int indent = 0) const;
};

SNF_DLL_DECLSPEC value from_string(const std::string &);
SNF_DLL_DECLSPEC value from_file(const std::string &);
SNF_DLL_DECLSPEC value from_stream(std::istream &);

} // json
} // snf

#endif // _SNF_JSON_H_
