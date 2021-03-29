/*
 * \brief  Implementation of some auxilary functions for golang runtime
 * \author Alexander Tormasov
 * \date   2020-11-17
 */

extern "C" {

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
	int sendfile(int fd, int s, off_t offset, size_t nbytes,
	             struct sf_hdtr *hdtr, off_t *sbytes, int flags)
	{
		return 0;
	}

#include <unistd.h>
	pid_t getpgid(pid_t pid) { return 0; }

	int lchown(const char *path, uid_t owner, gid_t group) { return 0; }
#include <sys/mount.h>
	int mount(const char *source, const char *target, int f, void *data)
	{
		return 0;
	}

#include <sys/time.h>
	int settimeofday(const struct timeval *tv , const struct timezone *tz)
	{
		return 0;
	}

#include <sys/mman.h>

	int mlock(const void *addr, size_t len) { return 0; }
	int munlock(const void *addr, size_t len) { return 0; }
	int mlockall(int flags) { return 0; }
	int munlockall(void) { return 0; }

#include <signal.h>
	int sigaltstack(const stack_t *ss, stack_t *oss) { return 0; }

	void __outgo_genode_log();
}

/* to produce call stack and halt */
#include <spec/x86_64/os/backtrace.h>
extern "C" int raise(int sig)
{
	Genode::log("raise ", sig);
	Genode::backtrace();
	return 0;
}

extern "C" void Log(const char * b) { Genode::log("Log: ",b); }

#include <util/string.h>

/* print to log from go using go string */
extern "C"
ssize_t __go_genode_write(int fd, const void *Buf, size_t count)
{
	static unsigned char b[1024];
	static unsigned int ptr=0; /* point to free byte */

	/* not safe in concurrent access */

	/* rest of buffer */
	const unsigned int size = sizeof(b)-1; /* available for data */
	unsigned int rest_buf;
	unsigned char *buf = (unsigned char *)Buf;
	again:	rest_buf = size - ptr;
	if (rest_buf) {
		while (count>0) {
			size_t to_copy = count > rest_buf ? rest_buf : count;
			Genode::memcpy(b+ptr,buf,to_copy);

			/* increment pointers */
			ptr += to_copy;
			count -= to_copy;
			buf += to_copy;
			rest_buf -= to_copy;

			/* print only if we have "\n" or no buf space */
			b[ptr] = 0; /* terminate string - safe because of size above */
			char *c;
			while ((c=(char*)__builtin_memchr(b,'\n',ptr)))	/* found, print */ {
				*c = 0; /* terminate string for ::log - replace \n */
				Genode::log("fd(", fd, "): ", (const char*)b);

				/* squeeze buffer */
				ptr -= c+1 - (char *)(&b[0]);
				rest_buf += c+1 - (char *)(&b[0]);
				Genode::memmove((void *)(&b[0]), c+1, ptr);
				b[ptr] = 0;
			}
		}
	} else {
		Genode::log("fd(", fd, "): ", (const char*)b);
		ptr = 0;
		goto again;
	}
	return count;
}

#include <base/thread.h>

/* base-internal includes */
// #include <base/internal/stack_allocator.h>
// #include <base/internal/globals.h>
#if 1
using namespace Genode;

/******************
 ** Startup code **
 ******************/

#include <libc/component.h>
#include <libc/args.h>

extern char **genode_argv;
extern int genode_argc;
extern char **genode_envp;

/* initial environment for the FreeBSD libc implementation */
extern char **environ;

/* provided by the application */
extern "C" int main(int argc, char **argv, char **envp);

static void construct_component(Libc::Env &env)
{
	populate_args_and_env(env, genode_argc, genode_argv, genode_envp);

	environ = genode_envp;

	exit(main(genode_argc, genode_argv, genode_envp));
}


namespace Libc {

	void anon_mmap_construct(Genode::Env &env, size_t default_size);
	void anon_init_file_operations(Genode::Env &env,
								   Xml_node const &config_accessor);
}

void Libc::Component::construct(Libc::Env &env)
{
	// Libc::anon_mmap_construct(env, 15ul * 1024 * 1024);
	Libc::anon_init_file_operations(env, env.libc_config());

	Libc::with_libc([&]() { construct_component(env); });
}
#endif
