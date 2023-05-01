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

#include <../net/mac80211/sta_info.h>

extern void ieee80211_eht_cap_ie_to_sta_eht_cap(struct ieee80211_sub_if_data * sdata,struct ieee80211_supported_band * sband,const u8 * he_cap_ie,u8 he_cap_len,const struct ieee80211_eht_cap_elem * eht_cap_ie_elem,u8 eht_cap_len,struct link_sta_info * link_sta);
void ieee80211_eht_cap_ie_to_sta_eht_cap(struct ieee80211_sub_if_data * sdata,struct ieee80211_supported_band * sband,const u8 * he_cap_ie,u8 he_cap_len,const struct ieee80211_eht_cap_elem * eht_cap_ie_elem,u8 eht_cap_len,struct link_sta_info * link_sta)
{
	lx_emul_trace_and_stop(__func__);
}

extern void ieee80211_link_init(struct ieee80211_sub_if_data * sdata,int link_id,struct ieee80211_link_data * link,struct ieee80211_bss_conf * link_conf);
void ieee80211_link_init(struct ieee80211_sub_if_data * sdata,int link_id,struct ieee80211_link_data * link,struct ieee80211_bss_conf * link_conf)
{
	lx_emul_trace_and_stop(__func__);
}


extern void ieee80211_link_setup(struct ieee80211_link_data * link);
void ieee80211_link_setup(struct ieee80211_link_data * link)
{
	lx_emul_trace_and_stop(__func__);
}


extern void ieee80211_link_stop(struct ieee80211_link_data * link);
void ieee80211_link_stop(struct ieee80211_link_data * link)
{
	lx_emul_trace_and_stop(__func__);
}

#include <net/mac80211.h>

int ieee80211_set_active_links(struct ieee80211_vif * vif,u16 active_links)
{
	lx_emul_trace_and_stop(__func__);
}


extern void ieee80211_vif_clear_links(struct ieee80211_sub_if_data * sdata);
void ieee80211_vif_clear_links(struct ieee80211_sub_if_data * sdata)
{
	lx_emul_trace_and_stop(__func__);
}


extern int ieee80211_vif_set_links(struct ieee80211_sub_if_data * sdata,u16 new_links);
int ieee80211_vif_set_links(struct ieee80211_sub_if_data * sdata,u16 new_links)
{
	lx_emul_trace_and_stop(__func__);
}