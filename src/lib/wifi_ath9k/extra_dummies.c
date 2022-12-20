#define KBUILD_MODNAME "extra_dummies"

#include <lx_emul.h>
#include <linux/types.h>
#include <net/sock.h>

u64 __sock_gen_cookie(struct sock *sk)
{
	lx_emul_trace_and_stop(__func__);
	return 0;
}

void *__vmalloc_node(unsigned long size, unsigned long align,
			    gfp_t gfp_mask, int node, const void *caller)
{
	lx_emul_trace_and_stop(__func__);
	return NULL;
}

#include <linux/filter.h>

int copy_bpf_fprog_from_user(struct sock_fprog *dst, sockptr_t src, int len)
{
	lx_emul_trace_and_stop(__func__);
	return 0;
}

char *get_options(const char *str, int nints, int *ints)
{
	lx_emul_trace_and_stop(__func__);
	return NULL;
}

#include <linux/workqueue.h>

void srcu_drive_gp(struct work_struct *wp)
{
	lx_emul_trace_and_stop(__func__);
}


#include <linux/fs.h>

int simple_setattr(struct user_namespace * my_user_namespace, struct dentry * dentry,struct iattr * iattr)
{
	lx_emul_trace_and_stop(__func__);
}
