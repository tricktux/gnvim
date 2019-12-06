/// @file process_handler.cpp
/// @brief Responsible for handling the neovim process
/// @author Reinaldo Molina
/// @version  0.0
/// @date Apr 15 2019
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

#include "reproc++/reproc.hpp"
#include "reproc++/sink.hpp"
#include <string>
#include <chrono>

#include "common.hpp"
#include "easylogging++.h"
#include "process_handler.hpp"

/// @brief Starts the process provided in @cmd
/// @param cmd Command to execute, includes arguments
/// @param timeout timeout in milliseconds
/// @return SUCCESS or <SUCCESS in case of error
int ReprocHandler::start(const std::vector<std::string> &cmd) {
  std::error_code ec;

  if (is_running()) {
    DLOG(WARNING) << "[" << __FUNCTION__ << "]: Process is running already";
    return SUCCESS;
  }

  if (cmd.empty()) {
    DLOG(ERROR) << "[" << __FUNCTION__ << "]: cmd argument empty";
    return -1;
  }

  DLOG(INFO) << "[" << __FUNCTION__ << "]: Cmd = ";
  for (const auto &c : cmd)
    DLOG(INFO) << "\t" << c;

  if ((ec = p.start(cmd))) {
    DLOG(ERROR) << "[" << __FUNCTION__ << "]: Failed to start process."
                << " ec.value = '" << ec.value() << "'."
                << " ec.message = '" << ec.value() << "'";
    if (ec == reproc::errc::file_not_found)
      DLOG(ERROR)
          << "[" << __FUNCTION__ << "]: "
          << "Program not found. Make sure it's available from the PATH.";
    return -2;
  }

  // TODO-[RM]-(Mon Apr 15 2019 15:08):
  // - Is this needed?
  p.close(reproc::stream::in);

  DLOG(INFO) << "[" << __FUNCTION__ << "]: ec.value = '" << ec.value() << "'";
  return ec.value();
}

/// @brief Stops the process @p initialized by the class
/// Note: Uses timeout value from class
/// @param timeout Time in milliseconds to wait for application to close
/// @return Process exit status
int ReprocHandler::stop(unsigned int timeout) {
  std::error_code ec;

  if (!is_running()) {
    DLOG(WARNING) << "[" << __FUNCTION__ << "]: Process stopped already";
    return p.exit_status();
  }

  DLOG(INFO) << "[" << __FUNCTION__ << "]: Stopping process";
  ec = p.stop(reproc::wait, reproc::milliseconds(timeout / 3),
              reproc::terminate, reproc::milliseconds(timeout / 3),
              reproc::kill, reproc::milliseconds(timeout / 3));

  if (ec) {
    DLOG(ERROR) << "[" << __FUNCTION__ << "]: Failed to stop the application"
                << " ec.value = '" << ec.value() << "'."
                << " ec.message = '" << ec.value() << "'";
  }

  return p.exit_status();
}