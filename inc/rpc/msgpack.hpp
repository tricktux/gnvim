/// @file msgpack.hpp
/// @brief Wrapper around MPack
/// @author Reinaldo Molina
/// @version  0.0
/// @date Dec 30 2019
// Copyright © 2019 Reinaldo Molina

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef MSGPACK_HPP
#define MSGPACK_HPP

#include "easylogging++.h"
#include "mpack.h"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>
#include <tuple>
#include <optional>

namespace nvimrpc {

void log_server_pack_node(mpack_node_t node);

// be careful of std::vector<Object> vs std::vector<ObjectWrapper>
// this can easily give bug
class ObjectWrapper;
using Object = std::variant<bool, int64_t, double, std::string, 
										 std::array<int64_t, 2>,
                     std::vector<ObjectWrapper>,
                     std::unordered_map<std::string, ObjectWrapper>>;

class ObjectWrapper {
  // Use shared_ptr not unique_ptr
  // The sematics of ObjectWrapper is a wrapper of an existing Object
  std::shared_ptr<Object> m_ptr;

  // Don't define operator Object ()
  // Since which returns a copy of the included Object

public:
  ObjectWrapper() = default;

  template <typename T> ObjectWrapper(T t) {
    m_ptr = std::make_shared<Object>(t);
  }

  Object &ref() { return *(m_ptr.get()); }
  const Object &ref() const { return *(m_ptr.get()); }
};

void inline mpack_write(mpack_writer_t *) {}

template <typename T, typename... Params>
void inline mpack_write(mpack_writer_t *writer, T &&value, Params &&... params);

class IMpackRpcPack {
public:
  const static size_t NUM_ELEMENTS = 4;
  const static size_t MSG_TYPE = 0;

  IMpackRpcPack() = default;
  virtual ~IMpackRpcPack() = default;

  virtual void set_msgid(size_t) = 0;
  virtual void set_method(std::string_view) = 0;
  // Cannot be virtual since it uses templates
  // template<class...Params> void set_params(Params&&... params);
  virtual std::string build() = 0;
};

/** @brief Nicely package a msgpack request
 *
 * @warning Class expects the following calling order:
 *	1. `set_msgid`
 *	2. `set_method`
 *	3. `set_params`
 *	4. `build`
 *
 * */
class MpackRpcPack : public IMpackRpcPack {
  char *data;
  size_t size;
  mpack_writer_t writer;

public:
  MpackRpcPack() : data(nullptr), size(0) {
    mpack_writer_init_growable(&writer, &data, &size);
    // The request message is a four elements array
    mpack_start_array(&writer, NUM_ELEMENTS);
    // The message type, must be the integer zero (0) for "Request" messages.
    mpack_write_i32(&writer, MSG_TYPE);
  }
  virtual ~MpackRpcPack() {
    if (data != nullptr) {
      MPACK_FREE(data);
      data = nullptr;
    }
  }

  /**
   * @brief Set rpc request msgid
   * This number is used as a sequence number. The server's response to the
   * "Request" will have the same msgid.
   * @param msgid A 32-bit unsigned integer number.
   */
  void set_msgid(size_t msgid) override { mpack_write_i32(&writer, msgid); }

  /**
   * @brief Set the method name
   * @param name A string which represents the method name.
   */
  void set_method(std::string_view name) override {
    if (name.empty()) {
      DLOG(ERROR) << "Empty method name";
      return;
    }
    mpack_write_cstr(&writer, name.data());
  }

  /**
   * @brief Pass the method parameters
   * @param params
   */
  template <class... Params> void set_params(Params &&... params) {
    mpack_start_array(&writer, sizeof...(Params));
    mpack_write(&writer, std::forward<Params>(params)...);
    mpack_finish_array(&writer);
  }
  std::string build() override;
};

class IMpackRpcUnpack {
public:
  ///
  const static size_t MAX_NUM_ELEMENTS = 10;
  const static size_t NOTIFICATION_NUM_ELEMENTS = 3;
  const static size_t NOTIFICATION_MSG_TYPE = 2;
  const static size_t RESPONSE_NUM_ELEMENTS = 4;
  const static size_t RESPONSE_MSG_TYPE = 1;
  const static size_t RESPONSE_MSG_TYPE_IDX = 0;
  const static size_t RESPONSE_MSG_ID_IDX = 1;
  const static size_t RESPONSE_ERROR_IDX = 2;
  const static size_t RESPONSE_RESULT_IDX = 3;
  const static size_t VECTOR_MAP_MAX_SIZE = 256;

  IMpackRpcUnpack() = default;
  virtual ~IMpackRpcUnpack() = default;

  virtual size_t get_num_elements() = 0;
  virtual int get_msg_type() = 0;
  virtual size_t get_msgid() = 0;
  virtual std::optional<std::tuple<int64_t, std::string>> get_error() = 0;
  // This function cannot be virtual because it uses templates
  // auto get_result();
};

template <typename T> T mpack_read(mpack_node_t);

class MpackRpcUnpack : public IMpackRpcUnpack {
  /// The node is valid until re-call `mpack_tree_try_parse`
  /// Nodes are immutable
  mpack_node_t &node;

public:
  const static size_t MAX_CSTR_SIZE = 1048576;

  MpackRpcUnpack(mpack_node_t &_node) : node(_node) {
    // log_server_pack_node(node);
  }
  virtual ~MpackRpcUnpack() = default;

  size_t get_num_elements() override {
    if (mpack_node_is_nil(node)) {
      DLOG(ERROR) << "Empty node";
      return 0;
    }
    return mpack_node_array_length(node);
  }

  int get_msg_type() override {
    if (mpack_node_is_nil(node)) {
      DLOG(ERROR) << "Empty node";
      return -1;
    }
    if (mpack_node_array_length(node) <= RESPONSE_MSG_TYPE_IDX) {
      DLOG(ERROR) << "Array in node is smaller than expected";
      return -2;
    }

    return mpack_node_u32(mpack_node_array_at(node, RESPONSE_MSG_TYPE_IDX));
  }

  size_t get_msgid() override {
    if (mpack_node_is_nil(node)) {
      DLOG(ERROR) << "Empty node";
      return 0;
    }
    if (mpack_node_array_length(node) <= RESPONSE_MSG_ID_IDX) {
      DLOG(ERROR) << "Array in node is smaller than expected";
      return 0;
    }

    return mpack_node_u32(mpack_node_array_at(node, RESPONSE_MSG_ID_IDX));
  }

  std::optional<std::tuple<int64_t, std::string>> get_error() override;

  // This function cannot be virtual because it uses templates
  template <typename T> T get_result() {
    if (mpack_node_is_nil(node)) {
      DLOG(ERROR) << "Empty node";
      return T();
    }

    if (mpack_node_array_length(node) <= RESPONSE_RESULT_IDX) {
      DLOG(ERROR) << "Array in node is smaller than expected";
      return T();
    }

    mpack_node_t result = mpack_node_array_at(node, RESPONSE_RESULT_IDX);
    if (mpack_node_is_nil(result)) {
      if (!std::is_void<T>::value)
        DLOG(WARNING) << "Got a nil result, but was expecting an actual value";
      return T();
    }

    return std::forward<T>(mpack_read<T>(result));
  }
};

// --------------mpack_write--------------------- //

void inline mpack_write(mpack_writer_t *writer, const Object &obj) {
  if (std::holds_alternative<bool>(obj))
    return mpack_write(writer, std::get<bool>(obj));
  if (std::holds_alternative<int64_t>(obj))
    return mpack_write(writer, std::get<int64_t>(obj));
  if (std::holds_alternative<double>(obj))
    return mpack_write(writer, std::get<double>(obj));
  if (std::holds_alternative<std::string>(obj))
    return mpack_write(writer, std::get<std::string>(obj));
  if (std::holds_alternative<std::array<int64_t, 2>>(obj))
    return mpack_write(writer, std::get<std::array<int64_t, 2>>(obj));
  if (std::holds_alternative<std::unordered_map<std::string, ObjectWrapper>>(
          obj)) {
    return mpack_write(
        writer, std::get<std::unordered_map<std::string, ObjectWrapper>>(obj));
  }
  if (std::holds_alternative<std::vector<ObjectWrapper>>(obj)) {
    return mpack_write(writer, std::get<std::vector<ObjectWrapper>>(obj));
  }

  DLOG(ERROR) << "Unrecognized Object type!";
}

void inline mpack_write(mpack_writer_t *writer, std::string_view value) {
  mpack_write_cstr(writer, value.data());
}
void inline mpack_write(mpack_writer_t *writer, const std::string &value) {
  mpack_write_cstr(writer, value.data());
}
// inline void
// mpack_write(mpack_writer_t *writer,
// const std::unordered_map<std::string, ObjectWrapper> &object_map);

void inline mpack_write(
    mpack_writer_t *writer,
    const std::unordered_map<std::string, Object> &object_map) {
  mpack_start_map(writer, object_map.size());
  for (const auto &val : object_map) {
    mpack_write(writer, val.first);
    mpack_write(writer, val.second);
  }
  mpack_finish_map(writer);
}

void inline mpack_write(
    mpack_writer_t *writer,
    const std::unordered_map<std::string, ObjectWrapper> &object_map) {
  mpack_start_map(writer, object_map.size());
  for (const auto &val : object_map) {
    mpack_write(writer, val.first);
    mpack_write(writer, val.second);
  }
  mpack_finish_map(writer);
}
void inline mpack_write(mpack_writer_t *writer,
                        const std::vector<ObjectWrapper> &object_list) {
  mpack_start_array(writer, object_list.size());
  for (const auto &val : object_list) {
    mpack_write(writer, val);
  }
  mpack_finish_array(writer);
}

template <typename T, typename... Params>
void inline mpack_write(mpack_writer_t *writer, T &&value,
                        Params &&... params) {
  mpack_write(writer, std::forward<T>(value));
  mpack_write(writer, std::forward<Params>(params)...);
}

// void mpack_write(mpack_writer_t *writer, const std::array<int64_t, 2> &val) {
// mpack_write_i64(writer, val[0]);
// mpack_write_i64(writer, val[1]);
// }

// --------------mpack_read--------------------- //
//
void inline check_node_type(mpack_node_t node, mpack_type_t expected_type) {
  mpack_tag_t tag = mpack_node_tag(node);
  auto type = mpack_tag_type(&tag);
  if (type != expected_type)
    DLOG(ERROR) << "Expected: '" << mpack_type_to_string(expected_type)
                << " node',  instead got: '" << mpack_type_to_string(type)
                << "'";
}
// template <typename T> T mpack_read(mpack_node_t);
template <> inline bool mpack_read<bool>(mpack_node_t node) {
  check_node_type(node, mpack_type_bool);
  return mpack_node_bool(node);
}
template <> inline int64_t mpack_read<int64_t>(mpack_node_t node) {
  check_node_type(node, mpack_type_int);
  return mpack_node_i64(node);
}
template <> inline uint64_t mpack_read<uint64_t>(mpack_node_t node) {
  check_node_type(node, mpack_type_uint);
  return mpack_node_u64(node);
}
template <> inline double mpack_read<double>(mpack_node_t node) {
  check_node_type(node, mpack_type_double);
  return mpack_node_double(node);
}
template <> inline std::string mpack_read<std::string>(mpack_node_t node) {
  check_node_type(node, mpack_type_str);
  const char *str = mpack_node_str(node);
  size_t len = mpack_node_strlen(node);
  return std::string{str, len};
}

template <>
inline std::array<int64_t, 2>
mpack_read<std::array<int64_t, 2>>(mpack_node_t node) {
  check_node_type(node, mpack_type_array);
  if (size_t l = mpack_node_array_length(node); l != 2) {
    DLOG(ERROR)
        << ": Try to read std::array<int64_t, 2> while mpack array size is: '"
        << l << "'";
  }

  std::array<int64_t, 2> res;
  res[0] = mpack_read<int64_t>(mpack_node_array_at(node, 0));
  res[1] = mpack_read<int64_t>(mpack_node_array_at(node, 1));
  return res;
}

template <typename T> std::vector<T> mpack_read_array(mpack_node_t node) {
  check_node_type(node, mpack_type_array);

  std::vector<T> res;
  size_t length = mpack_node_array_length(node);

  for (size_t index = 0; index < length; ++index) {
    auto curr_node = mpack_node_array_at(node, index);
    res.push_back(mpack_read<T>(curr_node));
  }
  return res;
}

template <typename K, typename V>
inline std::unordered_map<K, V> mpack_read_map(mpack_node_t node) {
  check_node_type(node, mpack_type_map);

  std::unordered_map<K, V> res;
  const size_t size = mpack_node_map_count(node);

  for (size_t index = 0; index < size; ++index) {
    auto curr_key = mpack_node_map_key_at(node, index);
    auto curr_val = mpack_node_map_value_at(node, index);
    res[mpack_read<K>(curr_key)] = mpack_read<V>(curr_val);
  }
  return res;
}

template <>
inline std::vector<std::string>
mpack_read<std::vector<std::string>>(mpack_node_t node) {
  return mpack_read_array<std::string>(node);
}

template <>
inline std::vector<Object> mpack_read<std::vector<Object>>(mpack_node_t node) {
  return mpack_read_array<Object>(node);
}

template <>
inline std::vector<int64_t>
mpack_read<std::vector<int64_t>>(mpack_node_t node) {
  return mpack_read_array<int64_t>(node);
}

template <>
inline std::vector<std::unordered_map<std::string, Object>>
mpack_read<std::vector<std::unordered_map<std::string, Object>>>(
    mpack_node_t node) {
  return mpack_read_array<std::unordered_map<std::string, Object>>(node);
}

template <>
inline std::unordered_map<std::string, Object>
mpack_read<std::unordered_map<std::string, Object>>(mpack_node_t node) {
  return mpack_read_map<std::string, Object>(node);
}

template <> inline Object mpack_read<Object>(mpack_node_t node) {
  Object obj{};
  auto tag = mpack_node_tag(node);
  switch (auto type = mpack_tag_type(&tag)) {
  case mpack_type_bool: {
    return mpack_read<bool>(node);
  }
  case mpack_type_int: {
    return mpack_read<int64_t>(node);
  }
  case mpack_type_uint: {
    return (int64_t)(mpack_read<uint64_t>(node));
  }
  case mpack_type_double: {
    return mpack_read<double>(node);
  }
  case mpack_type_str: {
    return mpack_read<std::string>(node);
  }
  case mpack_type_map: {
    auto res_map = mpack_read<std::unordered_map<std::string, Object>>(node);
    std::unordered_map<std::string, ObjectWrapper> res_map_wrapper;
    for (auto &item : res_map) {
      res_map_wrapper[item.first] = ObjectWrapper(item.second);
    }
    return Object(res_map_wrapper);
  }
  case mpack_type_array: {
    auto res_vec = mpack_read<std::vector<Object>>(node);
    std::vector<ObjectWrapper> res_vec_wrapper;
    for (auto &item : res_vec) {
      res_vec_wrapper.emplace_back(ObjectWrapper(item));
    }
    return Object(res_vec_wrapper);
  }
  default: {
    DLOG(ERROR) << "Unsupport type: '" << mpack_type_to_string(type) << "'";
    return {};
  }
  }
}

} // namespace nvimrpc
#endif
