#define HAVE_DIRENT_H 1
#define HAVE_DLFCN_H 1
//#define HAVE_GMODULE 1
#define HAVE_INTTYPES_H 1
#define HAVE_MEMORY_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRINGS_H 1
#define HAVE_STRING_H 1
#define HAVE_SYS_IOCTL_H 1
#define HAVE_SYS_RESOURCE_H 1
#define HAVE_SYS_SOCKET_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_SYS_UTSNAME_H 1
#define HAVE_UNISTD_H 1
#define PACKAGE_BUGREPORT ""
#define PACKAGE_NAME "irssi"
#define PACKAGE_STRING "irssi 1.2-head"
#define PACKAGE_TARNAME "irssi"
#define PACKAGE_URL ""
#define PACKAGE_VERSION "1.2-head"

#if __SIZEOF_LONG__ == 8
#define PRIuUOFF_T "lu"
#else
#define PRIuUOFF_T "llu"
#endif

#define SIZEOF_INT __SIZEOF_INT__
#define SIZEOF_LONG __SIZEOF_LONG__
#define SIZEOF_LONG_LONG __SIZEOF_LONG_LONG__
#define SIZEOF_OFF_T __SIZEOF_LONG__
#define STDC_HEADERS 1
#define UOFF_T_LONG 1
#define USE_GREGEX /**/

/*
 * HACK not sure why irssi uses this function in the first place
 *      since it should be internal to glib
 */
#define g_memmove(dest,src,len) G_STMT_START { memmove ((dest), (src), (len)); } G_STMT_END
