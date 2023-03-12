/**
* \brief  Wrapper for linux usb structures
* \author Colin Parker
* \date   2021-07-02
*/

/*
* Copyright (C) 2021 Colin Parker
*
* This file is distributed under the terms of the GNU General Public License
* version 2.
*/

#include <linux/usb.h>

static struct usb_driver lx_driver;
static int driver_registered = 0;
static void * cxx_context_ptr;

DECLARE_WAIT_QUEUE_HEAD(usb_kill_urb_queue);


int usb_register_driver(struct usb_driver * new_driver, struct module * owner,
                        const char * mod_name)
{
	(void) owner;
	(void) mod_name;

	if (driver_registered) return -1;

	printk("Genode emulation usb driver registered.\n");

	driver_registered = 1;
	lx_driver = *new_driver;

	return 0;
}


struct usb_device * usb_get_dev(struct usb_device *dev)
{
	if (dev)
		dev->dev.kobj.kref.refcount.refs.counter++;
	return dev;
}


void usb_put_dev(struct usb_device *dev)
{
	if (dev)
		dev->dev.kobj.kref.refcount.refs.counter--;
}

int cxx_usb_control_msg(void *, unsigned int, __u8, __u8, __u16, __u16, void *,
                        __u16, int);

int usb_control_msg(struct usb_device * dev, unsigned int pipe, __u8 request,
					__u8 requesttype, __u16 value, __u16 index, void * data,
					__u16 size, int timeout)
{
	return cxx_usb_control_msg(cxx_context_ptr, pipe, request, requesttype,
                               value, index, data, size, timeout);
}


void usb_init_urb(struct urb *urb)
{
	if (urb) {
		memset(urb, 0, sizeof(*urb));
		kref_init(&urb->kref);
		INIT_LIST_HEAD(&urb->urb_list);
		INIT_LIST_HEAD(&urb->anchor_list);
	}
}


enum { URB_ARRAY_SIZE = 256 };
static struct urb urb_array[URB_ARRAY_SIZE];
struct urb * usb_alloc_urb(int iso_packets, gfp_t mem_flags)
{
	static int urb_offset = 0;

	int i;
	int num_used = 0;
	
	(void) mem_flags;
	if (iso_packets > 0) {
		printk("Error: Isochronous USB not supported.");
		return NULL;
	}

	for (i = 0; i < URB_ARRAY_SIZE; ++i) {
		int elem = (i + urb_offset) % URB_ARRAY_SIZE;

		if (urb_array[elem].kref.refcount.refs.counter == 0) {
			urb_offset = (elem + 1) % URB_ARRAY_SIZE;
			usb_init_urb(&urb_array[elem]);
			atomic_set(&urb_array[elem].use_count, 0);
			return &urb_array[elem];
		}
		else num_used++;
	}
	printk("usb_alloc_urb: num_used is %d, urb_offset is %d.\n",
	       num_used, urb_offset);
	printk("Error: No more URBs available.\n");
	return NULL;
}


static void urb_destroy(struct kref * urb)
{
	(void)urb;
}


int cxx_usb_submit_urb(void * context_ptr, unsigned int pipe, void * buffer, u32 buf_size, void *urb);

int usb_submit_urb(struct urb *urb, gfp_t mem_flags)
{
	int return_val;
	(void) mem_flags;

	if ( !urb || !urb->complete) return -EINVAL;

	if ( atomic_read(&urb->reject) ) return -EPERM;

	usb_get_urb(urb);
	atomic_inc(&urb->use_count);

	return_val = cxx_usb_submit_urb(cxx_context_ptr,
	                                usb_pipeendpoint(urb->pipe),
	                                urb->transfer_buffer,
	                                urb->transfer_buffer_length, (void *)urb);
	if (return_val) {
		atomic_dec(&urb->use_count);
		usb_put_urb(urb);
	}
	return return_val;
}


void lx_usb_do_urb_callback(void * in_urb, int succeeded, int inbound, void * buffer, int buf_size)
{
	struct urb * urb = (struct urb *)in_urb;

	if (succeeded) {
		urb->status = 0;
		if (inbound) {
			memcpy(urb->transfer_buffer, buffer, buf_size);
		}
	}
	else urb->status = -1;
	urb->actual_length = buf_size;

	usb_unanchor_urb(urb);
	urb->complete(urb);
	atomic_dec(&urb->use_count);
	wake_up(&usb_kill_urb_queue);
	usb_put_urb(urb);
}

/* A single global device and its interface are allowed */
static struct usb_host_interface ath9k_host_if = { 0 };
static struct usb_device ath9k_usb_dev = { 0 };
static struct usb_interface ath9k_usb_if = { 0 };
static struct usb_device_id ath9k_dev_id = {USB_DEVICE(0, 0)};
static struct usb_host_config ath9k_host_cfg = { 0 };


int lx_usb_handle_connect(
	uint16_t vend_id,
	uint16_t prod_id,
	void * if_desc, /* usb_interface_descriptor * */
	void * cfg_desc, /* usb_config_descriptor * */
	void * ep_array,
	void * context_ptr)
{	
	static int device_registered = 0;
	static int interface_registered = 0;
	int err;

	cxx_context_ptr = context_ptr;

	/* Preparation of the usb_host_interface */
	memcpy(&ath9k_host_if.desc, if_desc, sizeof(struct usb_host_interface));
	ath9k_host_if.endpoint = ep_array;

	/* Prepartion of the usb_device */
	if ( !device_registered ) {
		ath9k_usb_dev.devnum = 7; /* lucky number */
		ath9k_usb_dev.actconfig = &ath9k_host_cfg;
		dev_set_name(&ath9k_usb_dev.dev, "ath9k_usb_dev");
		if ( ( err = device_register(&ath9k_usb_dev.dev) ) ) {
			printk("Device register failed.\n");
			return err;
		}
		device_registered = 1;
	}

	/* Preparation of the usb_interface */
	if ( !interface_registered ) {
		ath9k_usb_if.altsetting = &ath9k_host_if;
		ath9k_usb_if.cur_altsetting = &ath9k_host_if;
		ath9k_usb_if.num_altsetting = 1;
		ath9k_usb_if.usb_dev = (struct device *)&ath9k_usb_dev;
		ath9k_usb_if.dev.parent = &ath9k_usb_dev.dev;
		dev_set_name(&ath9k_usb_if.dev, "ath9k_usb_if");
		if ( ( err = device_register(&ath9k_usb_if.dev) ) ) {
			printk("Device register failed creating interface.\n");
			return err;
		}
		interface_registered = 1;
	}
	
	/* Preparation of the usb_device id */
	ath9k_dev_id.idVendor = vend_id;
	ath9k_dev_id.idProduct = prod_id;

	/* Preparation of the usb_host_config */
	memcpy(&ath9k_host_cfg.desc, cfg_desc, sizeof(struct usb_config_descriptor));
	ath9k_host_cfg.interface[0] = &ath9k_usb_if;

	return lx_driver.probe(&ath9k_usb_if, &ath9k_dev_id);
}


void lx_usb_handle_disconnect(void)
{
	lx_driver.disconnect(&ath9k_usb_if);
}


size_t lx_usb_host_endpoint_size(void)
{
	return sizeof(struct usb_host_endpoint);
}


void * lx_usb_host_to_epdesc(void *in_host)
{
	struct usb_host_endpoint *in_host_cast = (struct usb_host_endpoint *)in_host;
	return (void *)&in_host_cast->desc;
}


void lx_usb_setup_urb(void * urb, void * ep)
{
	struct urb * urb_ptr = (struct urb *)urb;

	urb_ptr->ep = (struct usb_host_endpoint *)ep;
}


static struct cred _usb_task_cred;

/* Based on init_task in lx_emul/start.c */
struct task_struct usb_comp_task = {
	.__state         = 0,
	.usage           = REFCOUNT_INIT(2),
	.flags           = PF_KTHREAD,
	.prio            = MAX_PRIO - 20,
	.static_prio     = MAX_PRIO - 20,
	.normal_prio     = MAX_PRIO - 20,
	.policy          = SCHED_NORMAL,
	.cpus_ptr        = &usb_comp_task.cpus_mask,
	.cpus_mask       = CPU_MASK_ALL,
	.nr_cpus_allowed = 1,
	.mm              = NULL,
	.active_mm       = NULL,
	.tasks           = LIST_HEAD_INIT(usb_comp_task.tasks),
	.real_parent     = &usb_comp_task,
	.parent          = &usb_comp_task,
	.children        = LIST_HEAD_INIT(usb_comp_task.children),
	.sibling         = LIST_HEAD_INIT(usb_comp_task.sibling),
	.group_leader    = &usb_comp_task,
	.comm            = "lxemul_usb",
	.thread          = INIT_THREAD,
	.pending         = {
		.list   = LIST_HEAD_INIT(usb_comp_task.pending.list),
		.signal = {{0}}
	},
	.blocked         = {{0}},
	.cred            = &_usb_task_cred,
};
void * lx_emul_usb_comp_struct = &usb_comp_task;

struct task_struct usb_conn_task = {
	.__state         = 0,
	.usage           = REFCOUNT_INIT(2),
	.flags           = PF_KTHREAD,
	.prio            = MAX_PRIO - 20,
	.static_prio     = MAX_PRIO - 20,
	.normal_prio     = MAX_PRIO - 20,
	.policy          = SCHED_NORMAL,
	.cpus_ptr        = &usb_conn_task.cpus_mask,
	.cpus_mask       = CPU_MASK_ALL,
	.nr_cpus_allowed = 1,
	.mm              = NULL,
	.active_mm       = NULL,
	.tasks           = LIST_HEAD_INIT(usb_conn_task.tasks),
	.real_parent     = &usb_conn_task,
	.parent          = &usb_conn_task,
	.children        = LIST_HEAD_INIT(usb_conn_task.children),
	.sibling         = LIST_HEAD_INIT(usb_conn_task.sibling),
	.group_leader    = &usb_conn_task,
	.comm            = "lxemul_usb",
	.thread          = INIT_THREAD,
	.pending         = {
		.list   = LIST_HEAD_INIT(usb_conn_task.pending.list),
		.signal = {{0}}
	},
	.blocked         = {{0}},
	.cred            = &_usb_task_cred,
};

void * lx_emul_usb_conn_struct = &usb_conn_task;

/* from drivers/usb/core/usb.c */
struct usb_interface *usb_ifnum_to_if(const struct usb_device *dev,
				      unsigned ifnum)
{
	struct usb_host_config *config = dev->actconfig;
	int i;

	if (!config)
		return NULL;
	for (i = 0; i < config->desc.bNumInterfaces; i++)
	{
		if (config->interface[i]->altsetting[0]
				.desc.bInterfaceNumber == ifnum)
			return config->interface[i];
	}

	return NULL;
}


/* These from usb/core/urb.c */

/**
 * usb_get_urb - increments the reference count of the urb
 * @urb: pointer to the urb to modify, may be NULL
 *
 * This must be  called whenever a urb is transferred from a device driver to a
 * host controller driver.  This allows proper reference counting to happen
 * for urbs.
 *
 * Return: A pointer to the urb with the incremented reference counter.
 */
struct urb *usb_get_urb(struct urb *urb)
{
	/*size_t addr_diff = (size_t)urb - (size_t)urb_array;*/
	
	if (urb)
		kref_get(&urb->kref);
	
	return urb;
}

/**
 * usb_free_urb - frees the memory used by a urb when all users of it are finished
 * @urb: pointer to the urb to free, may be NULL
 *
 * Must be called when a user of a urb is finished with it.  When the last user
 * of the urb calls this function, the memory of the urb is freed.
 *
 * Note: The transfer buffer associated with the urb is not freed unless the
 * URB_FREE_BUFFER transfer flag is set.
 */
void usb_free_urb(struct urb *urb)
{
	/*size_t addr_diff = (size_t)urb - (size_t)urb_array;*/
	
	if (urb)
		kref_put(&urb->kref, urb_destroy);
}

/**
 * usb_anchor_urb - anchors an URB while it is processed
 * @urb: pointer to the urb to anchor
 * @anchor: pointer to the anchor
 *
 * This can be called to have access to URBs which are to be executed
 * without bothering to track them
 */
void usb_anchor_urb(struct urb *urb, struct usb_anchor *anchor)
{
	unsigned long flags;

	spin_lock_irqsave(&anchor->lock, flags);
	usb_get_urb(urb);
	list_add_tail(&urb->anchor_list, &anchor->urb_list);
	urb->anchor = anchor;

	if (unlikely(anchor->poisoned))
		atomic_inc(&urb->reject);

	spin_unlock_irqrestore(&anchor->lock, flags);
}

static int usb_anchor_check_wakeup(struct usb_anchor *anchor)
{
	return atomic_read(&anchor->suspend_wakeups) == 0 &&
		list_empty(&anchor->urb_list);
}

/* Callers must hold anchor->lock */
static void __usb_unanchor_urb(struct urb *urb, struct usb_anchor *anchor)
{
	urb->anchor = NULL;
	list_del(&urb->anchor_list);
	usb_put_urb(urb);
	if (usb_anchor_check_wakeup(anchor))
		wake_up(&anchor->wait);
}

/**
 * usb_unanchor_urb - unanchors an URB
 * @urb: pointer to the urb to anchor
 *
 * Call this to stop the system keeping track of this URB
 */
void usb_unanchor_urb(struct urb *urb)
{
	unsigned long flags;
	struct usb_anchor *anchor;

	if (!urb)
		return;

	anchor = urb->anchor;
	if (!anchor)
		return;

	spin_lock_irqsave(&anchor->lock, flags);
	/*
	 * At this point, we could be competing with another thread which
	 * has the same intention. To protect the urb from being unanchored
	 * twice, only the winner of the race gets the job.
	 */
	if (likely(anchor == urb->anchor))
		__usb_unanchor_urb(urb, anchor);
	spin_unlock_irqrestore(&anchor->lock, flags);
}

/* above */
/* DECLARE_WAIT_QUEUE_HEAD(usb_kill_urb_queue); */

/**
 * usb_kill_urb - cancel a transfer request and wait for it to finish
 * @urb: pointer to URB describing a previously submitted request,
 *	may be NULL
 *
 * This routine cancels an in-progress request.  It is guaranteed that
 * upon return all completion handlers will have finished and the URB
 * will be totally idle and available for reuse.  These features make
 * this an ideal way to stop I/O in a disconnect() callback or close()
 * function.  If the request has not already finished or been unlinked
 * the completion handler will see urb->status == -ENOENT.
 *
 * While the routine is running, attempts to resubmit the URB will fail
 * with error -EPERM.  Thus even if the URB's completion handler always
 * tries to resubmit, it will not succeed and the URB will become idle.
 *
 * The URB must not be deallocated while this routine is running.  In
 * particular, when a driver calls this routine, it must insure that the
 * completion handler cannot deallocate the URB.
 *
 * This routine may not be used in an interrupt context (such as a bottom
 * half or a completion handler), or when holding a spinlock, or in other
 * situations where the caller can't schedule().
 *
 * This routine should not be called by a driver after its disconnect
 * method has returned.
 */
void usb_kill_urb(struct urb *urb)
{
	might_sleep();
	if (!(urb && urb->dev && urb->ep))
		return;
	atomic_inc(&urb->reject);

	wait_event(usb_kill_urb_queue, atomic_read(&urb->use_count) == 0);

	atomic_dec(&urb->reject);
}

/**
 * usb_kill_anchored_urbs - cancel transfer requests en masse
 * @anchor: anchor the requests are bound to
 *
 * this allows all outstanding URBs to be killed starting
 * from the back of the queue
 *
 * This routine should not be called by a driver after its disconnect
 * method has returned.
 */
void usb_kill_anchored_urbs(struct usb_anchor *anchor)
{
	struct urb *victim;

	spin_lock_irq(&anchor->lock);
	while (!list_empty(&anchor->urb_list)) {
		victim = list_entry(anchor->urb_list.prev, struct urb,
				    anchor_list);
		/* we must make sure the URB isn't freed before we kill it*/
		usb_get_urb(victim);
		spin_unlock_irq(&anchor->lock);
		/* this will unanchor the URB */
		usb_kill_urb(victim);
		usb_put_urb(victim);
		spin_lock_irq(&anchor->lock);
	}
	spin_unlock_irq(&anchor->lock);
}
