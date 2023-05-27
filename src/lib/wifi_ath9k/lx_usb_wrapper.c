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

#include <lx_emul/time.h>

enum {
	NUM_DRIVERS = 2,
};

static struct usb_driver lx_driver_array[NUM_DRIVERS] = { 0 };
static int driver_registered[NUM_DRIVERS] = { 0 };
static int connected_driver = -1;
static void * cxx_context_ptr;

DECLARE_WAIT_QUEUE_HEAD(usb_kill_urb_queue);

struct task_struct *usb_connect_task_struct_ptr;
struct task_struct *usb_complete_task_struct_ptr;
int    lx_usb_complete_task(void * in_wrap_struct);
int    lx_usb_connect_task(void * in_wrap_struct);

void lx_usb_init(void)
{
	pid_t pid;

	pid = kernel_thread(lx_usb_complete_task, &cxx_context_ptr,
	                    CLONE_FS | CLONE_FILES);
	usb_complete_task_struct_ptr = find_task_by_pid_ns(pid, NULL);
	pid = kernel_thread(lx_usb_connect_task, &cxx_context_ptr,
	                    CLONE_FS | CLONE_FILES);
	usb_connect_task_struct_ptr = find_task_by_pid_ns(pid, NULL);
}

void lx_usb_init_ctx(void * ctx)
{
	cxx_context_ptr = ctx;
}

int usb_register_driver(struct usb_driver * new_driver, struct module * owner,
                        const char * mod_name)
{
	int i;
	(void) owner;
	(void) mod_name;

	for (i = 0; i < NUM_DRIVERS; ++i) {
		if (driver_registered[i]) continue;

		driver_registered[i] = 1;
		lx_driver_array[i] = *new_driver;
		printk("Genode emulation usb driver registered for %s.\n",
		       new_driver->name);
		return 0;
	}
	printk("Error: No driver registration slots remaining after %d drivers"
	       "registered.\n", NUM_DRIVERS);
	return -1;
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
	/*printk("Control message, jiffies is %ld, HZ is %ld.\n", jiffies, (long)HZ);*/
	if (!cxx_context_ptr) return -ENXIO;
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
	unsigned int genode_pipe;
	(void) mem_flags;

	if ( !cxx_context_ptr ) return -ENXIO;

	if ( !urb || !urb->complete ) return -EINVAL;

	if ( atomic_read(&urb->reject) ) return -EPERM;

	/*printk("Submitting urb, old jiffies is %ld.\n", jiffies);*/
	lx_emul_time_handle();
	/*printk("Updated jiffies is %ld,\n", jiffies);*/

	usb_get_urb(urb);
	atomic_inc(&urb->use_count);

	genode_pipe = usb_pipeendpoint(urb->pipe);
	if (usb_pipein(urb->pipe)) genode_pipe |= USB_DIR_IN;

	return_val = cxx_usb_submit_urb(cxx_context_ptr,
	                                genode_pipe,
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
			int buf_copy_amt = buf_size;

			if (urb->transfer_buffer_length < buf_size )
				buf_copy_amt = urb->transfer_buffer_length;
			memcpy(urb->transfer_buffer, buffer, buf_copy_amt);
		}
	}
	else urb->status = -1;
	urb->actual_length = buf_size;

	/*printk("Calling the completion, jiffies is %ld.\n", jiffies);*/

	usb_unanchor_urb(urb);
	urb->complete(urb);
	atomic_dec(&urb->use_count);
	wake_up(&usb_kill_urb_queue);
	usb_put_urb(urb);
}

/* A single global device and its interface are allowed */
static struct usb_host_interface lx_usb_host_if;
static struct usb_device lx_usb_usb_dev;
static struct usb_interface lx_usb_usb_if;
static struct usb_device_id lx_usb_dev_id;
static struct usb_host_config lx_usb_host_cfg;


static void trivial_release(struct device *ignored) {
	(void) ignored;
}

int lx_usb_handle_connect(
	uint16_t vend_id,
	uint16_t prod_id,
	void * if_desc, /* usb_interface_descriptor * */
	void * cfg_desc, /* usb_config_descriptor * */
	void * ep_array)
{	
	int err;
	int i;
	struct usb_host_endpoint * ep0;

	/* Preparation of the usb_host_interface */
	lx_usb_host_if = (struct usb_host_interface){ 0 };
	memcpy(&lx_usb_host_if.desc, if_desc, sizeof(struct usb_interface_descriptor));
	ep0 = (struct usb_host_endpoint *)ep_array;
	lx_usb_host_if.endpoint = &ep0[1];


	ep0->desc.bLength = USB_DT_ENDPOINT_SIZE;
	ep0->desc.bDescriptorType = USB_DT_ENDPOINT;


	/* Prepartion of the usb_device */
	lx_usb_usb_dev = (struct usb_device){ 0 };
	lx_usb_usb_dev.devnum = 7; /* lucky number */
	lx_usb_usb_dev.actconfig = &lx_usb_host_cfg;
	lx_usb_usb_dev.dev.release = &trivial_release;
	dev_set_name(&lx_usb_usb_dev.dev, "lx_usb_usb_dev");
	if ( ( err = device_register(&lx_usb_usb_dev.dev) ) ) {
		printk("Device register failed.\n");
		return err;
	}

	/* Preparation of the usb_interface */
	lx_usb_usb_if = (struct usb_interface){ 0 };
	lx_usb_usb_if.altsetting = &lx_usb_host_if;
	lx_usb_usb_if.cur_altsetting = &lx_usb_host_if;
	lx_usb_usb_if.num_altsetting = 1;
	lx_usb_usb_if.usb_dev = (struct device *)&lx_usb_usb_dev;
	lx_usb_usb_if.dev.parent = &lx_usb_usb_dev.dev;
	lx_usb_usb_if.dev.release = &trivial_release;
	dev_set_name(&lx_usb_usb_if.dev, "lx_usb_usb_if");
	if ( ( err = device_register(&lx_usb_usb_if.dev) ) ) {
		printk("Device register failed creating interface.\n");
		return err;
	}
	
	/* Preparation of the usb_device id */
	lx_usb_dev_id = (struct usb_device_id){ 0 };
	lx_usb_dev_id.idVendor = vend_id;
	lx_usb_dev_id.idProduct = prod_id;

	/* Preparation of the usb_host_config */
	lx_usb_host_cfg = (struct usb_host_config){ 0 };
	memcpy(&lx_usb_host_cfg.desc, cfg_desc, sizeof(struct usb_config_descriptor));
	lx_usb_host_cfg.interface[0] = &lx_usb_usb_if;

	for (i = 0; i < NUM_DRIVERS; ++i) {
		if (!driver_registered[i]) continue;
		err = lx_driver_array[i].probe(&lx_usb_usb_if, &lx_usb_dev_id);
		if (!err) {
			printk("Successful probe of vendor_id: %04x and product_id %04x.\n", vend_id, prod_id);
			connected_driver = i;
			return 0;
		}
		if (err != -ENODEV)
			printk("Warning: Error %d in USB device probe.\n", err);
	}
	/* No driver can handle the device - not necessarily an error */
	printk("Warning: no driver found to handle device with vendor_id:"
		"%04x and product_id:%04x.\n", vend_id, prod_id);
	return 0;
}


void lx_usb_handle_disconnect(void)
{
	if (connected_driver < 0) return;
	printk("Disconnecting lx_driver_array[%d]\n", connected_driver);
	lx_driver_array[connected_driver].disconnect(&lx_usb_usb_if);
	connected_driver = -1;
	printk("The disconnect returned.\n");
	
	device_unregister(&lx_usb_usb_if.dev);
	device_unregister(&lx_usb_usb_dev.dev);
	cxx_context_ptr = NULL;
}


size_t lx_usb_host_endpoint_size(void)
{
	return sizeof(struct usb_host_endpoint);
}


void * lx_usb_host_to_epdesc(void *in_host)
{
	struct usb_host_endpoint *in_host_cast = (struct usb_host_endpoint *)in_host;
	if (!in_host) return NULL;
	else return (void *)&in_host_cast->desc;
}


void lx_usb_setup_urb(void * urb, void * ep)
{
	struct urb * urb_ptr = (struct urb *)urb;

	urb_ptr->ep = (struct usb_host_endpoint *)ep;
}


/*static struct cred _usb_task_cred;*/

/* Based on init_task in lx_emul/start.c */
/*struct task_struct usb_comp_task = {
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

void * lx_emul_usb_conn_struct = &usb_conn_task;*/

/* these from drivers/usb/core/usb.c */
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


static bool match_endpoint(struct usb_endpoint_descriptor *epd,
		struct usb_endpoint_descriptor **bulk_in,
		struct usb_endpoint_descriptor **bulk_out,
		struct usb_endpoint_descriptor **int_in,
		struct usb_endpoint_descriptor **int_out)
{
	switch (usb_endpoint_type(epd)) {
	case USB_ENDPOINT_XFER_BULK:
		if (usb_endpoint_dir_in(epd)) {
			if (bulk_in && !*bulk_in) {
				*bulk_in = epd;
				break;
			}
		} else {
			if (bulk_out && !*bulk_out) {
				*bulk_out = epd;
				break;
			}
		}

		return false;
	case USB_ENDPOINT_XFER_INT:
		if (usb_endpoint_dir_in(epd)) {
			if (int_in && !*int_in) {
				*int_in = epd;
				break;
			}
		} else {
			if (int_out && !*int_out) {
				*int_out = epd;
				break;
			}
		}

		return false;
	default:
		return false;
	}

	return (!bulk_in || *bulk_in) && (!bulk_out || *bulk_out) &&
			(!int_in || *int_in) && (!int_out || *int_out);
}

/**
 * usb_find_common_endpoints() -- look up common endpoint descriptors
 * @alt:	alternate setting to search
 * @bulk_in:	pointer to descriptor pointer, or NULL
 * @bulk_out:	pointer to descriptor pointer, or NULL
 * @int_in:	pointer to descriptor pointer, or NULL
 * @int_out:	pointer to descriptor pointer, or NULL
 *
 * Search the alternate setting's endpoint descriptors for the first bulk-in,
 * bulk-out, interrupt-in and interrupt-out endpoints and return them in the
 * provided pointers (unless they are NULL).
 *
 * If a requested endpoint is not found, the corresponding pointer is set to
 * NULL.
 *
 * Return: Zero if all requested descriptors were found, or -ENXIO otherwise.
 */
int usb_find_common_endpoints(struct usb_host_interface *alt,
		struct usb_endpoint_descriptor **bulk_in,
		struct usb_endpoint_descriptor **bulk_out,
		struct usb_endpoint_descriptor **int_in,
		struct usb_endpoint_descriptor **int_out)
{
	struct usb_endpoint_descriptor *epd;
	int i;

	if (bulk_in)
		*bulk_in = NULL;
	if (bulk_out)
		*bulk_out = NULL;
	if (int_in)
		*int_in = NULL;
	if (int_out)
		*int_out = NULL;

	for (i = 0; i < alt->desc.bNumEndpoints; ++i) {
		epd = &alt->endpoint[i].desc;

		if (match_endpoint(epd, bulk_in, bulk_out, int_in, int_out))
			return 0;
	}

	return -ENXIO;
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

u8 lx_usb_get_request(void * in_urb) {
	struct usb_ctrlrequest * ctrl_req;
	struct urb * urb = (struct urb *)in_urb;
	ctrl_req = (struct usb_ctrlrequest *)(urb->setup_packet);
	return ctrl_req->bRequest;
}
u8 lx_usb_get_request_type(void * in_urb) {
	struct usb_ctrlrequest * ctrl_req;
	struct urb * urb = (struct urb *)in_urb;
	ctrl_req = (struct usb_ctrlrequest *)(urb->setup_packet);
	return ctrl_req->bRequestType;
}
u16 lx_usb_get_value(void * in_urb) {
	struct usb_ctrlrequest * ctrl_req;
	struct urb * urb = (struct urb *)in_urb;
	ctrl_req = (struct usb_ctrlrequest *)(urb->setup_packet);
	return le16_to_cpu(ctrl_req->wValue);
}
u16 lx_usb_get_index(void * in_urb) {
	struct usb_ctrlrequest * ctrl_req;
	struct urb * urb = (struct urb *)in_urb;
	ctrl_req = (struct usb_ctrlrequest *)(urb->setup_packet);
	return le16_to_cpu(ctrl_req->wIndex);
}