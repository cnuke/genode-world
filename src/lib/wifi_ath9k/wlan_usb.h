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

#include <usb/usb.h>
#include <lx_kit/task.h>
#include <lx_kit/env.h>

using Genode::size_t;

extern "C" void * lx_emul_usb_comp_struct;
extern "C" void * lx_emul_usb_conn_struct;
extern "C" size_t lx_usb_host_endpoint_size(void);
extern "C" int    lx_usb_complete_task(void * in_wrap_struct);
extern "C" int    lx_usb_connect_task(void * in_wrap_struct);

namespace Genode {
	class Wlan_usb;
}

namespace Usb {
	class Lx_wrapper;
}

class Usb::Lx_wrapper : Usb::Completion
{
  private:
	Device & _dev;
	Genode::Entrypoint & _ep;
	Lx_kit::Task * _connect_task = nullptr;
	Lx_kit::Task * _complete_task = nullptr;
	Lx_kit::Task * _ctrl_task = nullptr;

	Lx_wrapper( Lx_wrapper & ) = delete;
	Lx_wrapper & operator=( Lx_wrapper & ) = delete;

	void * _endpoint_buffer = nullptr;
	int    _return_val = 0;

	enum ControlStatus {
		NONE, PENDING, COMPLETE,
	};

	ControlStatus _ctrl_status = NONE;
	Packet_descriptor _ctrl_packet = { };

	enum {
		PACKET_URB_MAP_SIZE = 256
	};

	Signal_handler<Lx_wrapper> app_completion_handler =
		{_ep, *this, &Lx_wrapper::app_handle_complete };

	struct Packet_urb_map
	{
		void            * urb;
		enum : Genode::uint8_t {
			AVAILABLE,
			PENDING,
			COMPLETE,
			QUEUED
		}                 complete;
		Genode::uint8_t   ep_index;
		Packet_descriptor p;
	} packet_map[PACKET_URB_MAP_SIZE] = { };
	int next_packet = 0;
	static int constexpr max_incoming = 1;
	static int constexpr ep_val_for_intr_in = 2;
	int pending_incoming = 0;
	int queued_incoming = 0;
	bool pending_connection = false;
	bool pending_disconnection = false;

	bool add_packet_with_urb(Packet_descriptor & p, void * urb, int ep_index);
	bool mark_packet_complete(Packet_descriptor & p);

	void app_handle_complete()
	{
		if ( _ctrl_status == COMPLETE ) {
			if(_ctrl_task) _ctrl_task->unblock();
			_ctrl_task = nullptr;
		}
		else if(_complete_task) _complete_task->unblock();
		Lx_kit::env().scheduler.schedule();
	}

  public:

	Lx_wrapper(Device & dev, Genode::Entrypoint & ep) : _dev(dev), _ep(ep) { }

	~Lx_wrapper()
	{
		destroy(Lx_kit::env().heap, _complete_task);
		destroy(Lx_kit::env().heap, _connect_task);
	}

	/* FIXME: set up tasks with kernel_thread for auto-assigned PIDs */
	static int constexpr default_pid = 1000;
	void init()
	{
		_complete_task = new (Lx_kit::env().heap)
			Lx_kit::Task(lx_usb_complete_task, (void *)this,
			             lx_emul_usb_comp_struct, default_pid,
			             "lxemul_usbcomp", Lx_kit::env().scheduler,
			             Lx_kit::Task::NORMAL);
		_connect_task = new (Lx_kit::env().heap)
			Lx_kit::Task(lx_usb_connect_task, (void *)this,
			             lx_emul_usb_conn_struct, default_pid+1,
			             "lxemul_usbconn", Lx_kit::env().scheduler,
			             Lx_kit::Task::NORMAL);
	}

	void send_completions();
	void complete(Usb::Packet_descriptor & p) override;
	int handle_connect(void *endpoint_buffer, int num_ep, bool connected);
	int handle_connect_internal();
	int usb_control_msg(unsigned int pipe, Genode::uint8_t request,
	                    Genode::uint8_t requesttype, Genode::uint16_t value,
	                    Genode::uint16_t index, void * data,
	                    Genode::uint16_t size, int timeout);

	int usb_submit_urb(unsigned int pipe, void * buffer, Genode::uint32_t buf_size, void * urb);
	void usb_submit_queued_urb(Packet_descriptor &p, int ep_index);

	Genode::size_t endpoint_desc_size()
	{
		return lx_usb_host_endpoint_size();
	}
};

struct Genode::Wlan_usb
{
	/* Genode Environment etc. */
	Env              & env;
	Heap               heap { env.ram(), env.rm() };
	Entrypoint       & ep { env.ep() };

	/* jitterentropy */
	struct rand_data * rnd_src { nullptr };

	/* USB */
	Signal_handler<Wlan_usb> state_change_dispatcher = {
		ep, *this, &Wlan_usb::handle_state_change };

	Allocator_avl          alloc_for_connection { &heap };
	Usb::Connection        usb { env, &alloc_for_connection, "wifi",
	                             4 * (1<<20), state_change_dispatcher };
	Usb::Device            device { &heap, usb, ep };

	Usb::Lx_wrapper        lx_usb_wrap { device, ep };

	void handle_state_change();

	Wlan_usb(Env &env) : env(env) {
		Genode::Signal_transmitter
			transmit_to_state_change_handler(state_change_dispatcher);
		transmit_to_state_change_handler.submit();
	}

	Wlan_usb(Wlan_usb &) = delete;
	Wlan_usb & operator=(Wlan_usb &) = delete;
};