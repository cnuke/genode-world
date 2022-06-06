/*
 * \brief  Libc functions for lwext4
 * \author Josef Soentgen
 * \date   2017-08-01
 */

/*
 * Copyright (C) 2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <base/allocator.h>
#include <base/log.h>
#include <base/snprintf.h>
#include <log_session/log_session.h>
#include <util/string.h>

/* library includes */
#include <lwext4/init.h>

/* compiler includes */
#include <stdarg.h>

/* local libc includes */
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*
 * Genode enviroment
 */
static Genode::Env       *_global_env;
static Genode::Allocator *_global_alloc;

void Lwext4::malloc_init(Genode::Env &env, Genode::Allocator &alloc)
{
	_global_env   = &env;
	_global_alloc = &alloc;
}


/*************
 ** stdio.h **
 *************/

int fflush(FILE *)
{
	return 0;
}


int printf(char const *fmt, ...)
{
	static char buffer[Genode::Log_session::MAX_STRING_LEN];

	va_list list;
	va_start(list, fmt);
	Genode::String_console sc(buffer, sizeof (buffer));
	sc.vprintf(fmt, list);
	va_end(list);

	/* remove newline as Genode::log() will implicitly add one */
	size_t n = sc.len();
	if (n > 0 && buffer[n-1] == '\n') { n--; }

	Genode::log(Genode::Cstring(buffer, n));

	return 0;
}


int puts(const char *s)
{
	printf("%s\n", s);
	return 0;
}


/**************
 ** stdlib.h **
 **************/

uint64_t __mem_allocs = 0;
uint64_t __mem_frees  = 0;
uint64_t __mem_alloc  = 0;
uint64_t __mem_free   = 0;

struct From
{
	void     *value;
	uint64_t  count;
};

struct Alloc_from
{
	From from[1024];
};

static bool inhibt_bt;

void *malloc(size_t sz)
{
	size_t *addr =
		reinterpret_cast<size_t*>(_global_alloc->alloc(sz + sizeof(size_t)));
	if (!addr)
		return nullptr;

	if (!inhibt_bt) {
	}

	++__mem_allocs;
	__mem_alloc += sz;

	*addr = sz;
	return (void*)++addr;
}


void *calloc(size_t n, size_t sz)
{
	/* XXX overflow check */
	size_t size = n * sz;
	inhibt_bt = true;
	void *p = malloc(size);
	inhibt_bt = false;
	if (p) { Genode::memset(p, 0, size); }
	return p;
}


void *realloc(void *p, size_t n)
{
	Genode::error(__func__, " not implemented");
	return NULL;
}


void free(void *p)
{
	if (p == NULL) { return; }

	size_t *v = reinterpret_cast<size_t*>(p);
	size_t *addr = --v;
	size_t sz    = *addr;

	++__mem_frees;
	__mem_free += sz;

	_global_alloc->free(addr, sz);
}


/**************
 ** string.h **
 **************/

int memcmp(void const *p1, void const *p2, size_t n)
{
	return Genode::memcmp(p1, p2, n);
}


void *memcpy(void *d, void const *s, size_t n)
{
	return Genode::memcpy(d, s, n);
}


void *memmove(void *d, void const *s, size_t n)
{
	return Genode::memmove(d, s, n);
}


void *memset(void *p, int c, size_t n)
{
	return Genode::memset(p, c, n);
}


int strcmp(char const *s1, char const *s2)
{
	return Genode::strcmp(s1, s2, ~0UL);
}


int strncmp(char const *s1, char const *s2, size_t n)
{
	return Genode::strcmp(s1, s2, n);
}


char *strcpy(char *d, char const *s)
{
	Genode::copy_cstring(d,s, ~0UL);
	return d;
}


char *strncpy(char *d, char const *s, size_t n)
{
	Genode::copy_cstring(d, s, n);
	return d;
}


size_t strlen(char const *s)
{
	return Genode::strlen(s);
}
