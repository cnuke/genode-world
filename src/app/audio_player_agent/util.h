/*
 * \brief   Audio Player Agent (utilities)
 * \author  Josef Soentgen
 * \date    2016-02-14
 */

/*
 * Copyright (C) 2016 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _UTIL_H_
#define _UTIL_H_

/* Genode includes */
#include <base/log.h>
#include <base/snprintf.h>
#include <util/xml_node.h>


/* Qt includes */
#include <QFile>


namespace Util {

	struct Buffer_guard;

	template <size_t CAPACITY>
	Genode::String<CAPACITY> string_attribute(Genode::Xml_node const &, char const *,
	                                          Genode::String<CAPACITY> const &);

	int64_t read_file(char const * const, char * const, size_t);
	int64_t write_file(char const * const, char const * const, size_t);

	char const *time_to_string(unsigned long);

	char const *normalize_path(char const *);
	bool        equal_path(char const *, char const *);
}


struct Util::Buffer_guard
{
	char const * const p;
	bool               array;
	Buffer_guard(char const * const p, bool array)
	: p(p), array(array) { }

	~Buffer_guard()
	{
		if (array) delete[] const_cast<char*>(p);
		else       delete   const_cast<char*>(p);
	}
};


template <Genode::size_t CAPACITY>
Genode::String<CAPACITY>
Util::string_attribute(Genode::Xml_node const &node, char const *attr,
                       Genode::String<CAPACITY> const &default_value)
{
	if (!node.has_attribute(attr)) return default_value;

	char buf[CAPACITY];
	node.attribute(attr).value(buf, sizeof(buf));
	return Genode::String<CAPACITY>(buf);
}


inline int64_t Util::read_file(char const * const name, char * const dst,
                               size_t len)
{
	QFile f(name);
	int64_t size = f.size();
	if (size > (int64_t)len) {
		Genode::error("destination buffer too small ", len,
		              " (need ", size, ") for reading '", name, "'",
		              len, size, name);
		throw -1;
	}

	if (!f.open(QIODevice::ReadOnly)) {
		Genode::error("could not open '", name, "'");
		return -1;
	}

	size = f.read(dst, len);
	if (size < 0) {
		Genode::error("could not read '", name, "'");
		return -1;
	}

	return size;
}


inline int64_t Util::write_file(char const * const name, char const * const src,
                      size_t len)
{
	QFile f(name);
	if (!f.open(QIODevice::WriteOnly)) {
		Genode::error("could not open '", name, "'");
		return -1;
	}

	int64_t size = f.write(src, len);
	if (size != (int64_t)len) {
		Genode::warning("could not write '", name, "' completely");
	}

	return size;
}


inline char const* Util::time_to_string(unsigned long t_ms)
{
	static char buffer[16];
	unsigned long const t_s = t_ms / 1000;
	unsigned      const   h = t_s / (60 * 60);
	unsigned      const   m = t_s / 60;
	unsigned      const   s = t_s - (60 * m);
	Genode::snprintf(buffer, sizeof(buffer), "%02u:%02u:%02u", h, m, s);
	return buffer;
}


inline char const *Util::normalize_path(char const *path)
{
	/* just omit the leading slash */
	return path + (path[0] == '/');
}


inline bool Util::equal_path(char const *p1, char const *p2)
{
	return Genode::strcmp(normalize_path(p1), normalize_path(p2)) == 0;
}

#endif /* _UTIL_H_ */
