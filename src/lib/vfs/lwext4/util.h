/*
 * \brief  VFS job helper utility
 * \author Josef Soentgen
 * \date   2022-01-03
 */

/*
 * Copyright (C) 2022 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _LWEXT4__UTIL_H_
#define _LWEXT4__UTIL_H_

/* Genode includes */
#include <vfs/file_io_service.h>

namespace Util
{
	using namespace Genode;

	class Number_of_bytes
	{
		uint64_t _n;

		public:

		Number_of_bytes() : _n(0) { }

		Number_of_bytes(Genode::uint64_t n) : _n(n) { }

		operator Genode::uint64_t() const { return _n; }

		void print(Output &output) const
		{
			using Genode::print;

			enum { KB = 1024UL, MB = KB*1024UL, GB = MB*1024UL };

			if      (_n      == 0) print(output, 0);
			else if (_n % GB == 0) print(output, _n/GB, "G");
			else if (_n % MB == 0) print(output, _n/MB, "M");
			else if (_n % KB == 0) print(output, _n/KB, "K");
			else                   print(output, _n);
		}
	};

	inline size_t ascii_to(const char *s, Number_of_bytes &result)
	{
		unsigned long res = 0;

		/* convert numeric part of string */
		int i = ascii_to_unsigned(s, res, 0);

		/* handle suffixes */
		if (i > 0)
			switch (s[i]) {
				case 'G': res *= 1024; [[fallthrough]];
				case 'M': res *= 1024; [[fallthrough]];
				case 'K': res *= 1024; i++;
				default: break;
			}

		result = res;
		return i;
	}
}

namespace Util
{
	struct Io;
	template <uint32_t> struct Bitmap;
	template <uint32_t> struct Cache;

	using addr_t = unsigned long;
	using off_t  = int64_t;
	using size_t = __SIZE_TYPE__;
};

struct Util::Io
{
	struct At
	{
		off_t value;
	};

	struct Buffer
	{
		uint8_t *base;
		size_t   length;
	};

	virtual ~Io() { }

	enum class Write_result { WRITE_OK, WRITE_ERR_IO };
	enum class Read_result  { READ_OK,  READ_ERR_IO };

	virtual Write_result write(Buffer const &src, At const at) = 0;

	virtual Read_result read(Buffer &dst, At const at) = 0;
};

template <uint32_t N>
struct Util::Bitmap
{
	static constexpr unsigned _bits  = (sizeof (addr_t) * 8);
	static constexpr unsigned _mask  = _bits - 1;
	static constexpr unsigned _count = N / _bits;

	addr_t _item[_count];

	Bitmap()
	{
		for (unsigned i = 0; i < _count; i++) {
			_item[i] = 0;
		}
	}

	bool used(uint32_t index) const
	{
		uint32_t const i      = index / _bits;
		uint32_t const offset = index & _mask;
		return _item[i] & (1ul << offset);
	}

	void set(uint32_t index)
	{
		uint32_t const i      = index / _bits;
		uint32_t const offset = index & _mask;

		_item[i] |= (1ul << offset);
	}

	void clear(uint32_t index)
	{
		uint32_t const i      = index / _bits;
		uint32_t const offset = index & _mask;
		_item[i] &= ~(1ul << offset);
	}

	void reset()
	{
		for (unsigned i = 0; i < _count; i++) {
			_item[i] = 0;
		}
	}

	bool any_set() const
	{
		bool result = false;
		for (unsigned i = 0; i < _count; i++) {
			result |= _item[i] != 0;
		}

		return result;
	}
};

namespace Util {

	using file_size = Vfs::file_size;
	using file_offset = Vfs::file_offset;

	struct Vfs_job
	{
		enum class Operation { READ, WRITE, SYNC };

		static char const *_op_to_string(Operation op)
		{
			switch (op) {
			case Operation::READ:  return "READ";
			case Operation::SYNC:  return "SYNC";
			case Operation::WRITE: return "WRITE";
			}
		}

		enum class State { PENDING, IN_PROGRESS, COMPLETE, };

		static char const *_state_to_string(State s)
		{
			switch (s) {
			case State::PENDING:     return "PENDING";
			case State::IN_PROGRESS: return "IN_PROGRESS";
			case State::COMPLETE:    return "COMPLETE";
			}
		}

		Vfs::Vfs_handle &_handle;

		Operation const   _op;
		char             * data;
		State              state;
		file_offset const  base_offset;
		file_offset        current_offset;
		file_size          current_count;

		bool success;
		bool complete;

		bool _read()
		{
			bool progress = false;

			switch (state) {
			case State::PENDING:

				_handle.seek(base_offset + current_offset);
				if (!_handle.fs().queue_read(&_handle, current_count))
					return progress;

				state = State::IN_PROGRESS;
				progress = true;
			[[fallthrough]];
			case State::IN_PROGRESS:
			{
				using Result = Vfs::File_io_service::Read_result;

				bool completed = false;
				file_size out = 0;

				Result const result =
					_handle.fs().complete_read(&_handle,
					                           data + current_offset,
					                           current_count, out);
				if (   result == Result::READ_QUEUED
				    || result == Result::READ_ERR_INTERRUPT
				    || result == Result::READ_ERR_AGAIN
				    || result == Result::READ_ERR_WOULD_BLOCK)
					return progress;
				else

				if (result == Result::READ_OK) {
					current_offset += out;
					current_count  -= out;
					success = true;
				} else

				if (   result == Result::READ_ERR_IO
				    || result == Result::READ_ERR_INVALID) {
					success   = false;
					completed = true;
				}

				if (current_count == 0 || completed)
					state = State::COMPLETE;
				else {
					state = State::PENDING;
					/* partial read, keep trying */
					return true;
				}
			}
			[[fallthrough]];
			case State::COMPLETE:

				complete = true;
				progress = true;
			default: break;
			}

			return progress;
		}

		bool _write()
		{
			bool progress = false;

			switch (state) {
			case State::PENDING:

				_handle.seek(base_offset + current_offset);
				state = State::IN_PROGRESS;
				progress = true;
			[[fallthrough]];
			case State::IN_PROGRESS:
			{
				using Result = Vfs::File_io_service::Write_result;

				bool completed = false;
				file_size out = 0;

				Result result = Result::WRITE_ERR_INVALID;
				try {
					result = _handle.fs().write(&_handle,
					                            data + current_offset,
					                            current_count, out);
				} catch (Vfs::File_io_service::Insufficient_buffer) {
					return progress;
				}

				if (   result == Result::WRITE_ERR_AGAIN
				    || result == Result::WRITE_ERR_INTERRUPT
				    || result == Result::WRITE_ERR_WOULD_BLOCK)
					return progress;
				else

				if (result == Result::WRITE_OK) {
					current_offset += out;
					current_count  -= out;
					success = true;
				} else

				if (   result == Result::WRITE_ERR_IO
					|| result == Result::WRITE_ERR_INVALID) {
					success = false;
					completed = true;
				}

				if (current_count == 0 || completed) {
					state = State::COMPLETE;
				} else {
					state = State::PENDING;
					/* partial write, keep trying */
					return true;
				}
			}
			[[fallthrough]];
			case State::COMPLETE:

				complete = true;
				progress = true;
			default: break;
			}

			return progress;
		}

		bool _sync()
		{
			bool progress = false;

			switch (state) {
			case State::PENDING:

				if (!_handle.fs().queue_sync(&_handle))
					return progress;

				state = State::IN_PROGRESS;
				progress = true;
			[[fallthrough]];
			case State::IN_PROGRESS:
			{
				using Result = Vfs::File_io_service::Sync_result;
				Result const result = _handle.fs().complete_sync(&_handle);

				if (result == Result::SYNC_QUEUED)
					return progress;
				else

				if (result == Result::SYNC_ERR_INVALID)
					success = false;
				else

				if (result == Result::SYNC_OK)
					success = true;

				state = State::COMPLETE;
				progress = true;
			}
			[[fallthrough]];
			case State::COMPLETE:

				complete = true;
				progress = true;
			default: break;
			}

			return progress;
		}

		bool _trim()
		{
			/*
 			 * TRIM is not implemented but nonetheless report success
			 * back to client as it merely is a hint.
			 */
			success  = true;
			complete = true;
			return true;
		}

		Vfs_job(Vfs::Vfs_handle &handle,
		        Operation        op,
		        file_offset      base_offset,
		        char            *data,
		        file_size        length)
		:
			_handle        { handle },
			_op            { op },
			data           { data },
			state          { State::PENDING },
			base_offset    { base_offset },
			current_offset { 0 },
			current_count  { length },
			success        { false },
			complete       { false }
		{ }

		bool completed() const { return complete; }
		bool succeeded() const { return success; }

		void print(Genode::Output &out) const
		{
			Genode::print(out,
				"op: ",             _op_to_string(_op), " "
				"state: ",          _state_to_string(state), " "
				"base_offset: ",    base_offset, " "
				"current_offset: ", current_offset, " "
				"current_count: ",  current_count, " "
				"success: ",        success, " "
				"complete: ",       complete);
		}

		bool execute()
		{
			switch (_op) {
			case Operation::READ:  return _read();
			case Operation::SYNC:  return _sync();
			case Operation::WRITE: return _write();
			default:               return false;
			}
		}
	};

} /* namespace Util */

#endif /* _LWEXT4__UTIL_H_ */
