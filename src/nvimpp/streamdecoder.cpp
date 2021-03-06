/// @file streamdecoder.cpp
/// @brief Class in charge of parsing stream output
/// @author Reinaldo Molina
/// @version  0.0
/// @date Dec 19 2019
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

#include "nvimpp/streamdecoder.hpp"

#include "easylogging++.h"

/**
 * @brief Parse data from the `stdout` into an `mpack_node_t`
 * This function uses `mpack_tree_try_parse` which in turn calls `read_iodev`,
 * which in turn calls `iodevice->recv`, which then asynchronously retrieves
 * data in `output` string.
 * `mpack_tree_root` converts the `mpack_tree_t` into a `mpack_node_t`
 * See @ref ReprocDevice::recv
 * @return `mpack_node_t`
 */
std::optional<mpack_node_t> nvimrpc::StreamDecoder::poll() {
  if (mpack_tree_try_parse(&tree)) {
    return mpack_tree_root(&tree);
  }
  // mpack_tree_parse(&tree);

  // no message valid, two possible reasons:
  //   1. error occurred
  //   2. no sufficent data received
  mpack_error_t ec = mpack_tree_error(&tree);
  if (ec == mpack_ok) {
    return {};
    // return mpack_tree_root(&tree);
  }

  std::string err{"Error: "};
  err.append(mpack_error_to_string(ec));
  LOG(FATAL) << err;
  throw std::runtime_error(err.c_str());
  return {};
}

inline size_t nvimrpc::StreamDecoder::read_iodev(mpack_tree_t *ptree, char *buf,
                                                 size_t count) {
  if (ptree == nullptr) {
    LOG(ERROR) << "Invalid tree pointer";
    return 0;
  }
  if (buf == nullptr) {
    LOG(ERROR) << "Invalid buf pointer";
    return 0;
  }
  if (count == 0) {
    LOG(ERROR) << "Zero count provided";
    return 0;
  }

  auto piodev = (IoDevice *)(mpack_tree_context(ptree));
  return piodev->read(buf, count);
}
