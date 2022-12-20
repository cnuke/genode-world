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

#include <lx_emul/task.h>

#include "wlan_usb.h"

extern "C" void   lx_usb_do_urb_callback(void * urb, int suceeded, int inbound,
                                         void * buffer, int buf_size);
extern "C" int    lx_usb_handle_connect(Genode::uint16_t vend_id,
                                        Genode::uint16_t prod_id,
                                        void * if_desc, void * cfg_desc,
                                        void * ep_array, void * context_ptr);
extern "C" void   lx_usb_handle_disconnect();										
extern "C" void * lx_usb_host_to_epdesc(void * in_host);
extern "C" void   lx_usb_setup_urb(void * urb, void * ep);

void Genode::Wlan_usb::handle_state_change()
{
	static void * lx_ep_buffer = nullptr;
	static size_t lx_ep_buffer_size = 0;
	static bool inited = false;
	int num_ep = 0;
	
	if (!inited) {
		lx_usb_wrap.init();
		inited = true;
	}

	if (!usb.plugged()) {
		log("Device unplugged");
	}
	else {
		log("Device plugged");
		device.update_config();

		Usb::Interface &iface = device.interface(0);

		try { iface.claim(); }
		catch (Usb::Session::Interface_already_claimed) {
			error("Device already claimed");
			return;
		} catch (Usb::Session::Interface_not_found) {
			error("Interface not found");
			return;
		}

		num_ep = iface.current().num_endpoints;
		if ( lx_ep_buffer_size != num_ep*lx_usb_wrap.endpoint_desc_size()) {
			if ( lx_ep_buffer_size > 0) {
				warning("Number of endpoints changed between connection!");
				heap.free(lx_ep_buffer, lx_ep_buffer_size);
			}
			
			lx_ep_buffer_size = num_ep*lx_usb_wrap.endpoint_desc_size();

			try { lx_ep_buffer = heap.alloc(lx_ep_buffer_size); }
			catch (Genode::Out_of_caps) {
				error("Failed to allocate buffer for linux endpoint data"
				" (out of caps).");
				return;
			}
			catch (Genode::Out_of_ram) {
				error("Failed to allocate buffer for linux endpoint data"
				      " (out of ram).");
				return;
			}
			catch (Ram_allocator::Denied) {
				error("Failed to allocate buffer for linux endpoint data"
				      " (denied).");
				return;
			}
		}
	}
	int handle_result = lx_usb_wrap.handle_connect(lx_ep_buffer, num_ep,
	                                               usb.plugged());

	log("Result of handle_connect: ", handle_result);
}

bool Usb::Lx_wrapper::add_packet_with_urb(Packet_descriptor & p, void * urb,
                                          int ep_index )
{
	int i;

	for (i = 0; i < PACKET_URB_MAP_SIZE; ++i) {
		int search = (i + next_packet) % PACKET_URB_MAP_SIZE;
		if ( packet_map[search].complete == Packet_urb_map::AVAILABLE ) {
			next_packet = (search + 1) % PACKET_URB_MAP_SIZE;
			packet_map[search].p = p;
			packet_map[search].urb = urb;
			packet_map[search].ep_index = (Genode::uint8_t)ep_index;

			if (ep_index == ep_val_for_intr_in) {
				/* incoming interrupt must be queued */
				if ( pending_incoming < max_incoming ) {
					pending_incoming++;
					packet_map[search].complete = Packet_urb_map::PENDING;
					return false;
				}
				else {
					queued_incoming++;
					packet_map[search].complete = Packet_urb_map::QUEUED;
					return true;
				}
			}
			else {
				packet_map[search].complete = Packet_urb_map::PENDING;
				return false;
			}
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
		int search = (i + next_packet) % PACKET_URB_MAP_SIZE;
		void * search_packet = iface.content(packet_map[search].p);
		if ((search_packet == packet) &&
		    (packet_map[search].complete == Packet_urb_map::PENDING) )
		{
			packet_map[search].complete = Packet_urb_map::COMPLETE;
			packet_map[search].p        = p;
			return true;
		}
	}
	return false;
}

void Usb::Lx_wrapper::send_completions()
{
	int i;
	int saved_next_packet;

	/* guard against device being disconnected */
	if ( !_dev.config ) return; 
	Usb::Interface & iface = _dev.interface(0);

	do {
		saved_next_packet = next_packet;
		for (i = 0; i < PACKET_URB_MAP_SIZE; ++i) {
			int search = (i + saved_next_packet) % PACKET_URB_MAP_SIZE;
			if ( packet_map[search].complete == Packet_urb_map::COMPLETE) {
				void * packet = iface.content(packet_map[search].p);
				lx_usb_do_urb_callback(packet_map[search].urb,
									packet_map[search].p.succeded,
									packet_map[search].p.transfer.ep & 0x80,
									packet,
									packet_map[search].p.transfer.actual_size);
				iface.release(packet_map[search].p);
				if ( packet_map[search].ep_index == ep_val_for_intr_in ) {
					pending_incoming--;
				}
				packet_map[search].complete = Packet_urb_map::AVAILABLE;
			}
			else if ( pending_incoming < max_incoming &&
						packet_map[search].complete == Packet_urb_map::QUEUED)
			{
				packet_map[search].complete = Packet_urb_map::PENDING;
				queued_incoming--;
				pending_incoming++;
				usb_submit_queued_urb(packet_map[search].p,
				                      packet_map[search].ep_index);
			}
		}
	} while (pending_incoming < max_incoming && queued_incoming > 0);
}

void Usb::Lx_wrapper::complete(Usb::Packet_descriptor & p)
{	
	Usb::Interface &iface = _dev.interface(0);

	void * packet_content = iface.content(p);
	if ( (packet_content == nullptr) ||
	     (!mark_packet_complete(p)) ) {
		if (_ctrl_status == PENDING) {
			_ctrl_status = COMPLETE;
			_ctrl_packet = p;
		}
		else {
			Genode::log("No record of packet and no pending control transfer.");
			Genode::log("Packet error is ", (int)p.error, " and succeded is ",
			            p.succeded);
		}
	}
	Genode::Signal_transmitter transmit_to_app_handler(app_completion_handler);
	transmit_to_app_handler.submit();
}

int Usb::Lx_wrapper::handle_connect(void *endpoint_buffer, int num_ep,
                                    bool connected)
{
	/* assumes that endpoint_buffer is allocated with the right size */
	if (endpoint_buffer == nullptr)
	{
		return true;
	}
	else _endpoint_buffer = endpoint_buffer;

	if (!connected) {
		pending_disconnection = true;
	}
	else {
		for (int i = 0; i < num_ep; ++i)
		{
			char *buffer_base = (char *)_endpoint_buffer
			                    + i*lx_usb_host_endpoint_size();

			auto buffer_desc = (Endpoint_descriptor *) lx_usb_host_to_epdesc(
				(void *)buffer_base );

			*buffer_desc = _dev.interface(0).current().endpoint(i);
		}

		pending_connection = true;
	}

	if(_connect_task) _connect_task->unblock();
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
		                             _endpoint_buffer, this);
	}
	else if (pending_disconnection ) {
		pending_disconnection = false;
		lx_usb_handle_disconnect();
	}
	return 0;
}

int Usb::Lx_wrapper::usb_control_msg(unsigned int pipe, Genode::uint8_t request,
                                     Genode::uint8_t requesttype,
                                     Genode::uint16_t value,
                                     Genode::uint16_t index, void * data,
                                     Genode::uint16_t size, int timeout)
{
	(void) pipe; /* Genode doesn't support control transfer to non-default EP? */

	if ( _ctrl_status != NONE ) {
		Genode::error("Attempt to submit a synchronous control message before"
		              " the previous one completed.");
		throw -1;
	}

	/* Check if device disconnected */
	if ( !_dev.config ) return -1;
	Usb::Interface &iface = _dev.interface(0);

	Usb::Packet_descriptor p = iface.alloc(size);
	Genode::memcpy(iface.content(p), data, size);
	iface.control_transfer(p, requesttype, request, value, index, timeout,
	                       false, this);
	_ctrl_status = PENDING;
	_ctrl_task = &Lx_kit::env().scheduler.current();
	while (_ctrl_status == PENDING) {
		lx_emul_task_schedule(true);
	}
	_ctrl_status = NONE;

	if (!_ctrl_packet.succeded) {
		Genode::error("Error in usb_control_msg.");
		return -1;
	}
	else {
		return size;
	}
}

int Usb::Lx_wrapper::usb_submit_urb(unsigned int pipe, void * buffer,
                                    Genode::uint32_t buf_size, void * urb)
{
	/* check against disconnected device */
	if ( !_dev.config ) return -1;
	Usb::Interface &iface = _dev.interface(0);
	int num_ep = iface.current().num_endpoints;
	int ep_search;
	int i;

	for (i = 0; i < num_ep; ++i) {
		ep_search = (i + pipe - 1) % num_ep;
		if ((iface.current().endpoint(ep_search).address & 0xF) == pipe) break;
	}
	if (i >= num_ep) {
		Genode::error("Attempt to send on a pipe ( ", pipe,
		              " ) I couldn't find.");
		return -1;
	}

	Endpoint & ep = iface.current().endpoint(ep_search);

	Packet_descriptor p = iface.alloc(buf_size);
	if ( !(ep.address & 0x80) ) { /* outgoing */
		Genode::memcpy(iface.content(p), (char *)buffer, buf_size);
	}
	lx_usb_setup_urb(urb, &ep);

	if ( add_packet_with_urb(p, urb, ep_search) ) return 0;
	if (ep.interrupt()) {
		iface.interrupt_transfer(
			p, ep, Usb::Packet_descriptor::DEFAULT_POLLING_INTERVAL, false,
			this );
		return 0;
	}
	else if (ep.bulk()) {
		iface.bulk_transfer( p, ep, false, this );
		return 0;
	}
	return -1;
}

void Usb::Lx_wrapper::usb_submit_queued_urb(Packet_descriptor &p, int ep_index)
{
	Usb::Interface &iface = _dev.interface(0);
	Usb::Endpoint &ep = iface.current().endpoint(ep_index);
	iface.interrupt_transfer(
		p, ep, Usb::Packet_descriptor::DEFAULT_POLLING_INTERVAL, false, this );
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
	Usb::Lx_wrapper * cxx_wrap_struct = (Usb::Lx_wrapper *) in_wrap_struct;

	while(true) {
		cxx_wrap_struct->send_completions();
		lx_emul_task_schedule(true);
	}
	/* never reached */
	return 0;
}

extern "C" int lx_usb_connect_task(void * in_wrap_struct)
{
	Usb::Lx_wrapper * cxx_wrap_struct = (Usb::Lx_wrapper *) in_wrap_struct;

	while(true) {
		cxx_wrap_struct->handle_connect_internal();
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