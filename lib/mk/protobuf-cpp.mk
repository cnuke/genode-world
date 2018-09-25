PROTOBUF_CPP_PORT_DIR := $(call select_from_ports,protobuf-cpp)

PROTOBUF_CPP_SRC_DIR := \
	$(PROTOBUF_CPP_PORT_DIR)/src/lib/protobuf-cpp/src/google/protobuf

INC_DIR += $(PROTOBUF_CPP_PORT_DIR)/src/lib/protobuf-cpp/src/

SRC_CC := any.cc \
          any.pb.cc \
          api.pb.cc \
          arena.cc \
          arenastring.cc \
          descriptor.cc \
          descriptor.pb.cc \
          descriptor_database.cc \
          duration.pb.cc \
          dynamic_message.cc \
          empty.pb.cc \
          extension_set.cc \
          extension_set_heavy.cc \
          field_mask.pb.cc \
          generated_message_reflection.cc \
          generated_message_table_driven.cc \
          generated_message_table_driven_lite.cc \
          generated_message_util.cc \
          implicit_weak_message.cc \
          map_field.cc \
          message.cc \
          message_lite.cc \
          reflection_ops.cc \
          repeated_field.cc \
          service.cc \
          source_context.pb.cc \
          struct.pb.cc \
          text_format.cc \
          timestamp.pb.cc \
          type.pb.cc \
          unknown_field_set.cc \
          wire_format.cc \
          wire_format_lite.cc \
          wrappers.pb.cc

SRC_CC += compiler/importer.cc \
          compiler/parser.cc

SRC_CC += io/coded_stream.cc \
          io/gzip_stream.cc \
          io/printer.cc \
          io/strtod.cc \
          io/tokenizer.cc \
          io/zero_copy_stream.cc \
          io/zero_copy_stream_impl.cc \
          io/zero_copy_stream_impl_lite.cc \

SRC_CC += util/delimited_message_util.cc \
          util/field_comparator.cc \
          util/field_mask_util.cc \
          util/internal/datapiece.cc \
          util/internal/default_value_objectwriter.cc \
          util/internal/error_listener.cc \
          util/internal/field_mask_utility.cc \
          util/internal/json_escaping.cc \
          util/internal/json_objectwriter.cc \
          util/internal/json_stream_parser.cc \
          util/internal/object_writer.cc \
          util/internal/proto_writer.cc \
          util/internal/protostream_objectsource.cc \
          util/internal/protostream_objectwriter.cc \
          util/internal/type_info.cc \
          util/internal/type_info_test_helper.cc \
          util/internal/utility.cc \
          util/json_util.cc \
          util/message_differencer.cc \
          util/time_util.cc \
          util/type_resolver_util.cc

SRC_CC += stubs/bytestream.cc \
          stubs/common.cc \
          stubs/int128.cc \
          stubs/io_win32.cc \
          stubs/mathlimits.cc \
          stubs/status.cc \
          stubs/statusor.cc \
          stubs/stringpiece.cc \
          stubs/stringprintf.cc \
          stubs/structurally_valid.cc \
          stubs/strutil.cc \
          stubs/substitute.cc \
          stubs/time.cc

LIBS += libc stdcxx stdcxx_supplements

vpath %.cc $(PROTOBUF_CPP_SRC_DIR)

CC_CXX_WARN_STRICT =
