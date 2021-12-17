/*
 * \brief  Block device backend for lwext4
 * \author Josef Soentgen
 * \date   2022-01-03
 */

/*
 * Copyright (C) 2022 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _LWEXT4__BLOCK_H_
#define _LWEXT4__BLOCK_H_

/* Genode includes */
#include <base/log.h>
#include <os/reporter.h>
#include <vfs/env.h>
#include <vfs/vfs_handle.h>
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

/* lwext4 includes */
#include <ext4.h>
#include <ext4_blockdev.h>

#include "util.h"


namespace Lwext4 {

	using Cache = Util::Cache<512>;

	struct Block_device;
	Block_device &block_init(Vfs::Env &, char const *, uint32_t, size_t);
	void          block_deinit(Vfs::Env &, Block_device&);
	void          block_sync(Block_device&);

	void block_cache_stats(Block_device &, Genode::Reporter::Xml_generator &);
};

struct Lwext4::Block_device
{
	struct ext4_blockdev       ext4_blockdev;
	struct ext4_blockdev_iface ext4_blockdev_iface;
	unsigned char              ext4_block_buffer[4096];

	uint32_t block_size;

	Vfs::Env                             *_vfs_env;
	Vfs::Vfs_handle                      *_vfs_handle;
	Genode::Constructible<Util::Vfs_job>  _vfs_job { };

	Cache *_cache;
};

#endif /* _LWEXT4__BLOCK_H_ */
