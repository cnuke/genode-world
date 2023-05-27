/**
* \brief  WLAN structure USB extensions
* \author Colin Parker
* \date   2022-06-04
*/

/*
* Copyright (C) 2022 Colin Parker
*
* This file is distributed under the terms of the GNU General Public License
* version 2.
*/

/*#define WLAN_USB_VERBOSE*/

/** Notes - places where the Linux scheduler can be invoked 
 * 
 * repos/dde_linux:
 * In Lx_kit::Timeout::_handle
 * 		this is probably safe because the time handler runs
 * In Device::Irq::_handle()
 * 		hopefully this never happens for a USB driver
 * In Genode::Global_irq_controller::trigger_irq
 * 		hopefully this never happens for a USB driver
 * In lx_emul_execute_kernel_until
 * 		only called from the USB HC driver AFAICT
 * In lx_emul_start_kernel
 * 		only called once at the beginning
 * 
 * repos/world/src/lib/wifi_ath9k
 * In Lx::Socket::_handle
 * 		this is a risk and maybe should be protected
 * In wifi_kick_socketcall
 * 		this is a risk and maybe should be protected
 * In Usb::Lx_wrapper::send_and_receive
 * 		this is a risk and maybe should be protected
 * In Usb::Lx_wrapper::handle_connect
 * 		this is a small risk but maybe should still be protected
 * In _wifi_set_rfkill (x2)
 * 		this is a small risk but maybe should still be protected
 * In Wlan::_handle_signal()
 * 		this is a risk and maybe should be protected
*/

#include <lx_emul/task.h>

#include "wlan_usb.h"

extern "C" void   lx_usb_do_urb_callback(void * urb, int suceeded, int inbound,
                                         void * buffer, int buf_size);
extern "C" int    lx_usb_handle_connect(Genode::uint16_t vend_id,
                                        Genode::uint16_t prod_id,
                                        void * if_desc, void * cfg_desc,
                                        void * ep_array);
extern "C" void   lx_usb_handle_disconnect();										
extern "C" void * lx_usb_host_to_epdesc(void * in_host);
extern "C" void   lx_usb_setup_urb(void * urb, void * ep);
extern "C" void   lx_usb_init(void);
extern "C" void   lx_usb_init_ctx(void * ctx);
extern "C" Genode::uint8_t  lx_usb_get_request(void * urb);
extern "C" Genode::uint8_t  lx_usb_get_request_type(void * urb);
extern "C" Genode::uint16_t lx_usb_get_value(void * urb);
extern "C" Genode::uint16_t lx_usb_get_index(void * urb);

void Genode::Wlan_usb::handle_state_change()
{
	switch (status) {
	case NOTARGET:
		usb_device_rom.update();

		if (!usb_device_rom.valid()) { return; }
		else {
			Genode::Xml_node config = usb_device_rom.xml();
			usb_target_name = { 0 };
			config.with_optional_sub_node("device",
										[&] (Genode::Xml_node const &node) {
				usb_target_name =
					node.attribute_value("label", target_name_str());
			});
		}
		if (usb_target_name.length()) {
			Genode::log("Attempting to connect to device ", usb_target_name);
			usb.construct(env,
			              &alloc_for_connection,
			              usb_target_name.string(),
			              4 * (1<<20),
			              state_change_dispatcher);
			if (usb->plugged()) {
				status = DISCONNECTED;
				[[fallthrough]];
			}
			else {
				Genode::log("Connected but device unplugged.");
				break;
			}
		}
		else break;
	case DISCONNECTED:
		if (usb->plugged()) {
			Genode::log("Device plugged.");
			device.construct(&heap, *usb, ep);
			lx_usb_wrap.construct(*device, ep,
				Genode::Signal_transmitter(state_change_dispatcher));
			
			device->update_config();
			Usb::Interface &iface = device->interface(0);

			try { iface.claim(); }
			catch (Usb::Session::Interface_already_claimed) {
				error("Device already claimed");
				break;
			} catch (Usb::Session::Interface_not_found) {
				error("Interface not found");
				break;
			}

			int num_ep = iface.current().num_endpoints;
			if ( lx_ep_buffer_size != (num_ep+1)*lx_usb_wrap->endpoint_desc_size()) {
				if ( lx_ep_buffer_size > 0) {
					warning("Number of endpoints changed between connections!");
					heap.free(lx_ep_buffer, lx_ep_buffer_size);
				}
				
				lx_ep_buffer_size = (num_ep+1)*lx_usb_wrap->endpoint_desc_size();

				try { lx_ep_buffer = heap.alloc(lx_ep_buffer_size); }
				catch (Genode::Out_of_caps) {
					error("Failed to allocate buffer for linux endpoint data"
					" (out of caps).");
					break;
				}
				catch (Genode::Out_of_ram) {
					error("Failed to allocate buffer for linux endpoint data"
						" (out of ram).");
					break;
				}
				catch (Ram_allocator::Denied) {
					error("Failed to allocate buffer for linux endpoint data"
						" (denied).");
					break;
				}
				Genode::memset(lx_ep_buffer, 0, lx_ep_buffer_size);
			}
			Genode::log("Calling lx_usb_wrap->handle_connect(...)");
			lx_usb_wrap->handle_connect(lx_ep_buffer, num_ep, true);
			status = CONNECTED;
			break;
		}
	case CONNECTED:
		if (!usb->plugged()) {
			Genode::log("Device unplugged.");
		}
		lx_usb_wrap->handle_connect(nullptr, 0, false);

		status = DISCONNECTING;
		break;
	case DISCONNECTING:
		if (!lx_usb_wrap->pending_disconnection()) {
			lx_usb_wrap.destruct();
			device.destruct();
			usb.destruct();
			status = NOTARGET;
		}
	}
	if (status == DISCONNECTED && device.constructed()) {
		device.destruct();
		lx_usb_wrap.destruct();
	}
	if (status == DISCONNECTED && !usb_device_rom.valid()) {
		usb.destruct();
		status = NOTARGET;
	}
}

void Usb::Lx_wrapper::add_packet_with_urb(Packet_descriptor & p, void * urb,
                                          int ep_index, bool incoming )
{
	int i;

	for (i = 0; i < PACKET_URB_MAP_SIZE; ++i) {
		int search = (i + next_packet) % PACKET_URB_MAP_SIZE;
		if ( packet_map[search].complete == Packet_urb_map::AVAILABLE ) {
			next_packet = (search + 1) % PACKET_URB_MAP_SIZE;
			packet_map[search].p = p;
			packet_map[search].urb = urb;
			packet_map[search].ep_index = (Genode::uint8_t)ep_index;

			queued_packets[ep_index]++;
			if (incoming) {
				int places_available = max_incoming - pending_packets[ep_index];
				if (queued_packets[ep_index] <= places_available)
					available_work++;
			}
			else available_work++;
			packet_map[search].complete = Packet_urb_map::QUEUED;
			Genode::Signal_transmitter transmitter(send_recv_handler);
			transmitter.submit();
			#ifdef WLAN_USB_VERBOSE
			Genode::log("Packet added in position ", search, " at ", Lx_kit::env().timer.curr_time().trunc_to_plain_us(), " ep_index ", ep_index,
				" queued_packets ", queued_packets[ep_index], " incoming ", incoming);
			#endif
			return;
		}
	}
	Genode::error("Attempt to add more packets than queue allows.");
	throw -1;
}

bool Usb::Lx_wrapper::mark_packet_complete(Packet_descriptor & p)
{
	int i;
	Usb::Interface & iface = _dev.interface(0);
	void * packet = iface.content(p);

	for (i = PACKET_URB_MAP_SIZE; i > 0; --i) {
		int search = (i + next_packet - 1) % PACKET_URB_MAP_SIZE;
		void * search_packet = iface.content(packet_map[search].p);
		if ((search_packet == packet) &&
		    (packet_map[search].complete == Packet_urb_map::PENDING) )
		{
			packet_map[search].complete = Packet_urb_map::COMPLETE;
			packet_map[search].p        = p;
			Genode::uint8_t ep_index = packet_map[search].ep_index;
			bool incoming = false;
			if (ep_index == MAX_ENDPOINTS - 1) incoming = true;
			else if (ep_index < MAX_ENDPOINTS - 2) incoming = p.read_transfer();
			if (incoming) {
				int places_available = max_incoming - pending_packets[ep_index]--;
				/* increment available work if there were more queued packets than 
				* available space */
				if (queued_packets[ep_index] > places_available)
					available_work++;
			}
			#ifdef WLAN_USB_VERBOSE
				Genode::log("Marking complete packet in position ", search, " ep_index ", ep_index, " pending_packets ", pending_packets[ep_index]);
			#endif
			return true;
		}
	}
	return false;
}

void Usb::Lx_wrapper::send_and_receive()
{
	int i;

	/* guard against device being disconnected */
	Usb::Interface * iface;
	if ( !_dev.config ) {
		iface = nullptr;
		/* recompute available work */
		available_work = 0;
		for (i = 0; i < MAX_ENDPOINTS; ++i) {
			available_work += queued_packets[i];
		}
	}
	else {
		iface = &_dev.interface(0);
	}

	/* process as much sending as we can */
	for (i = 0; available_work > 0; ++i) {
		int search = (i + next_packet + 1) % PACKET_URB_MAP_SIZE;
		if ( packet_map[search].complete == Packet_urb_map::QUEUED) {
			if (iface) {
				int ep_index = packet_map[search].ep_index;
				bool incoming = false;

				if (ep_index < MAX_ENDPOINTS - 2) {
					Endpoint & ep = iface->current().endpoint(ep_index);
					incoming = ep.address & 0x80;
				}
				else if (ep_index == MAX_ENDPOINTS - 1) {
					incoming = false;
				}
				else incoming = true;

				if (!incoming || pending_packets[ep_index] < max_incoming) {
					packet_map[search].complete = Packet_urb_map::PENDING;
					pending_packets[ep_index]++;
					queued_packets[ep_index]--;
					available_work--;
					#ifdef WLAN_USB_VERBOSE
						Genode::log("Submitting queued packet in position ", search, " at ", Lx_kit::env().timer.curr_time().trunc_to_plain_us(),
							" ep_index ", ep_index, " queued_packets ", queued_packets[ep_index], " pending packets ", pending_packets[ep_index], " incoming ", incoming);
					#endif
					Endpoint *ep = nullptr;
					if (ep_index < MAX_ENDPOINTS - 2) {
						ep = &iface->current().endpoint(ep_index);
					}
					usb_submit_queued_urb(packet_map[search].p, ep);
				}
			}
			else {
				/* disconnected, just return an error */
				packet_map[search].complete = Packet_urb_map::COMPLETE;
				packet_map[search].p.succeded = false;
			}
		}
		if (i > 0 && i % (10*PACKET_URB_MAP_SIZE) == 0) {
			Genode::warning("Probably stuck in send_and_receive() with available_work ", available_work);
			for (int j = 0; j < MAX_ENDPOINTS; ++j) {
				Genode::warning("pending_packets[", j, "] = ", pending_packets[j], ", queued_packets[", j, "] = ", queued_packets[j]);
			}
		}
	}
	

	/* send completions and handle control transfers */
	if ( _ctrl_status == PENDING_OUT ) {
		Usb::Interface &iface = _dev.interface(0);

		_ctrl_status = PENDING_IN;
		iface.control_transfer(_ctrl_packet, _ctrl_out_data.requesttype,
		                       _ctrl_out_data.request, _ctrl_out_data.value,
		                       _ctrl_out_data.index, _ctrl_out_data.timeout,
		                       false, this);
	}
	else if(_ctrl_status == COMPLETE) {
		if (_ctrl_task) _ctrl_task->unblock();
		_ctrl_task = nullptr;
	}
		
	if(usb_complete_task_struct_ptr)
		lx_emul_task_unblock(usb_complete_task_struct_ptr);
	Lx_kit::env().scheduler.unblock_time_handler();
	Lx_kit::env().scheduler.schedule();
}

void Usb::Lx_wrapper::send_completions()
{
	int i;

	/* guard against device being disconnected */
	Usb::Interface * iface;
	if ( !_dev.config )
		iface = nullptr;
	else
		iface = &_dev.interface(0);

	for (i = 0; i < PACKET_URB_MAP_SIZE; ++i) {
		int search = (i + next_packet + 1) % PACKET_URB_MAP_SIZE;
		if ( packet_map[search].complete == Packet_urb_map::COMPLETE) {
			bool incoming = false;
			int actual_size;
			if (packet_map[search].ep_index >= MAX_ENDPOINTS - 2) {
				incoming = packet_map[search].ep_index == MAX_ENDPOINTS - 1;
				actual_size = packet_map[search].p.control.actual_size;
			}
			else if (packet_map[search].ep_index < MAX_ENDPOINTS - 2) {
				incoming = packet_map[search].p.read_transfer();
				actual_size = packet_map[search].p.transfer.actual_size;
			}
			if (iface) {
				void * packet = iface->content(packet_map[search].p);
				#ifdef WLAN_USB_VERBOSE
					Genode::log("Completion submitted in position ", search, " at ", Lx_kit::env().timer.curr_time().trunc_to_plain_us());
				#endif
				lx_usb_do_urb_callback(packet_map[search].urb,
									packet_map[search].p.succeded,
									incoming, packet, actual_size);
				iface->release(packet_map[search].p);
			}
			else { /* iface is invalid, send completion with error */
				lx_usb_do_urb_callback(packet_map[search].urb,
				                       false,
				                       incoming,
				                       nullptr, 0);
			}
			packet_map[search].complete = Packet_urb_map::AVAILABLE;
		}
	}
}

void Usb::Lx_wrapper::complete(Usb::Packet_descriptor & p)
{	
	
	Usb::Interface &iface = _dev.interface(0);

	void * packet_content = iface.content(p);
	if ( (packet_content == nullptr) ||
	     (!mark_packet_complete(p)) ) {
		if (_ctrl_status == PENDING_IN) {
			_ctrl_status = COMPLETE;
			_ctrl_packet = p;
		}
		else {
			Genode::warning("No record of packet and no pending control transfer.");
			Genode::warning("Packet error is ", (int)p.error, " and succeded is ",
			            p.succeded);
		}
	}
	Genode::Signal_transmitter transmitter(send_recv_handler);
	transmitter.submit();
}

int Usb::Lx_wrapper::handle_connect(void *endpoint_buffer, int num_ep,
                                    bool connected)
{
	if (!connected) {
		_pending_disconnection = true;
		send_and_receive();
	}
	else {
		_endpoint_buffer = endpoint_buffer;

		for (int i = 0; i < num_ep; ++i)
		{
			auto buffer_desc = (Endpoint_descriptor *) lx_usb_host_to_epdesc(
				endpoint_desc_at_index(i) );

			*buffer_desc = _dev.interface(0).current().endpoint(i);
		}

		pending_connection = true;
	}

	lx_usb_init_ctx(this);
	if (usb_connect_task_struct_ptr)
		lx_emul_task_unblock(usb_connect_task_struct_ptr);
	Lx_kit::env().scheduler.unblock_time_handler();
	Lx_kit::env().scheduler.schedule();

	return _return_val;
}

int Usb::Lx_wrapper::handle_connect_internal()
{
	if ( pending_connection ) {
		pending_connection = false;

		Interface_descriptor *ath9k_idesc = &_dev.interface(0).current();
		Config_descriptor *ath9k_cdesc = _dev.config;

		return lx_usb_handle_connect(_dev.device_descr.vendor_id,
		                             _dev.device_descr.product_id,
		                             (void *)ath9k_idesc, (void *)ath9k_cdesc,
		                             _endpoint_buffer);
	}
	else if (_pending_disconnection ) {
		lx_usb_handle_disconnect();
		_pending_disconnection = false;
		_pstate_chg.submit();
	}
	return 0;
}

int Usb::Lx_wrapper::usb_control_msg(unsigned int pipe, Genode::uint8_t request,
                                     Genode::uint8_t requesttype,
                                     Genode::uint16_t value,
                                     Genode::uint16_t index, void * data,
                                     Genode::uint16_t size, int timeout)
{
	if ( _ctrl_status != NONE ) {
		Genode::error("Attempt to submit a synchronous control message before"
		              " the previous one completed.");
		throw -1;
	}

	/* Check if device disconnected */
	if ( !_dev.config ) return -1;
	Usb::Interface &iface = _dev.interface(0);

	Usb::Packet_descriptor p = iface.alloc(size);
	if (!(pipe & Usb::ENDPOINT_IN))
		Genode::memcpy(iface.content(p), data, size);
	_ctrl_status = PENDING_OUT;
	_ctrl_packet = p;
	/* The control transfer will call wait_and_dispatch_one_io_signal. This in
	 * turn can trigger timeouts, which get then get run on our stack. This can
	 * subsequently run the Linux scheduler, creating an invalid stack.
	 * Therefore, what we have to do is schedule an application-level Genode
	 * handler, and break out of Linux emulation with lx_emul_task_schedule. */
	_ctrl_out_data = { request, requesttype, value, index, timeout };
	Genode::Signal_transmitter transmit_to_app_handler(send_recv_handler);
	transmit_to_app_handler.submit();
	/*iface.control_transfer(p, requesttype, request, value, index, timeout,
	                       false, this);*/
	while (_ctrl_status != COMPLETE) {
		_ctrl_task = &Lx_kit::env().scheduler.current();
		lx_emul_task_schedule(true);
	}
	_ctrl_status = NONE;

	if (!_ctrl_packet.succeded) {
		Genode::error("Error in usb_control_msg.");
		return -1;
	}
	
	if (pipe & Usb::ENDPOINT_IN)
		Genode::memcpy(data, iface.content(p), size);
	return size;
}

extern "C" void lx_emul_trace_and_stop(const char *);
int Usb::Lx_wrapper::usb_submit_urb(unsigned int pipe, void * buffer,
                                    Genode::uint32_t buf_size, void * urb)
{
	#ifdef WLAN_USB_VERBOSE
		Genode::log("Submitting urb.");
	#endif
	/* check against disconnected device */
	if ( !_dev.config ) return -1;
	Usb::Interface &iface = _dev.interface(0);
	int num_ep = iface.current().num_endpoints;
	int ep_search = 0;
	int i;
	bool incoming;
	Packet_descriptor p;

	for (i = 0; i < num_ep; ++i) {
		ep_search = (i + (pipe & 0xF) - 1) % num_ep;
		if (iface.current().endpoint(ep_search).address == pipe) break;
	}
	if (i >= num_ep) {
		if (pipe & 0xF) {
			Genode::error("Attempt to send on a pipe ( ", Genode::Hex(pipe),
			              " ) I couldn't find.");
			return -1;
		}
		/* Here we have endpoint 0 control transactions */
		lx_usb_setup_urb(urb, endpoint_0_desc());
		p = iface.alloc(buf_size);
		/* setup p */
		p.type                 = Usb::Packet_descriptor::CTRL;
		p.succeded             = false;
		p.control.request      = lx_usb_get_request(urb);
		p.control.request_type = lx_usb_get_request_type(urb);
		p.control.value        = lx_usb_get_value(urb);
		p.control.index        = lx_usb_get_index(urb);
		p.control.timeout      = 0;
		p.completion           = this;

		incoming = pipe & 0x80;

		if (incoming) {
			ep_search = MAX_ENDPOINTS - 1; /* value indicating incoming ep0 */
		}
		else {
			Genode::memcpy(iface.content(p), (char *)buffer, buf_size);
			ep_search = MAX_ENDPOINTS - 2; /* value indicating outgoing ep0 */
		}
	}
	else {
		Endpoint & ep = iface.current().endpoint(ep_search);
		lx_usb_setup_urb(urb, endpoint_desc_at_index(ep_search));
		
		p = iface.alloc(buf_size);
		incoming = ep.address & 0x80;
		if ( !(incoming) ) { /* outgoing */
			Genode::memcpy(iface.content(p), (char *)buffer, buf_size);
		}
	}

	add_packet_with_urb(p, urb, ep_search, incoming);

	return 0;
	/*if (ep.interrupt()) {
		iface.interrupt_transfer(
			p, ep, Usb::Packet_descriptor::DEFAULT_POLLING_INTERVAL, false,
			this );
		return 0;
	}
	else if (ep.bulk()) {
		iface.bulk_transfer( p, ep, false, this );
		return 0;
	}
	return -1;*/
}

void Usb::Lx_wrapper::usb_submit_queued_urb(Packet_descriptor &p, Endpoint *ep)
{
	Usb::Interface &iface = _dev.interface(0);
	if (ep == nullptr) { /* control transfer */
		iface.submit(p);
	}
	else if (ep->interrupt()) {
		iface.interrupt_transfer(
			p, *ep, Usb::Packet_descriptor::DEFAULT_POLLING_INTERVAL, false,
			this );
	}
	else if (ep->bulk()) {
		iface.bulk_transfer( p, *ep, false, this );
	}
	/* do not support isoch endpoints... */
}

extern "C" int cxx_usb_control_msg(void *context_ptr, unsigned int pipe,
                                   Genode::uint8_t request,
                                   Genode::uint8_t requesttype,
                                   Genode::uint16_t value,
                                   Genode::uint16_t index, void * data,
                                   Genode::uint16_t size, int timeout)
{
	Usb::Lx_wrapper * lx_wrap = (Usb::Lx_wrapper *)context_ptr;
	return lx_wrap->usb_control_msg(pipe, request, requesttype, value, index,
	                                data, size, timeout);
}

extern "C" int lx_usb_complete_task(void * in_wrap_struct)
{
	Usb::Lx_wrapper * cxx_wrap_struct;

	while(true) {
		cxx_wrap_struct = *((Usb::Lx_wrapper **) in_wrap_struct);
		if (cxx_wrap_struct) cxx_wrap_struct->send_completions();
		lx_emul_task_schedule(true);
	}
	/* never reached */
	return 0;
}

extern "C" int lx_usb_connect_task(void * in_wrap_struct)
{
	Usb::Lx_wrapper * cxx_wrap_struct;
	
	while(true) {
		cxx_wrap_struct = *((Usb::Lx_wrapper **) in_wrap_struct);
		if (cxx_wrap_struct) cxx_wrap_struct->handle_connect_internal();
		lx_emul_task_schedule(true);
	}
	/* never reached */
	return 0;
}

extern "C" int cxx_usb_submit_urb(void * context_ptr, unsigned int pipe,
                                  void * buffer, Genode::uint32_t buf_size,
                                  void * urb)
{
	Usb::Lx_wrapper * lx_wrap = (Usb::Lx_wrapper *)context_ptr;
	return lx_wrap->usb_submit_urb(pipe, buffer, buf_size, urb);
}