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

#pragma once

#include <string>
#include <vector>

class IProcessHandler {
protected:
  IProcessHandler() {}
	virtual ~IProcessHandler() {}

public:
  virtual int start(const std::vector<std::string> &cmd) = 0;
  virtual int stop(unsigned int timeout) = 0;
  virtual bool is_running() = 0;
};

class ReprocHandler : public IProcessHandler {
  reproc::process p;

public:
  ReprocHandler(unsigned int _timeout)
      : p(reproc::terminate, reproc::milliseconds(_timeout/2), reproc::kill,
           reproc::milliseconds(_timeout/2)) {}
  virtual ~ReprocHandler() {
    if (is_running())
      p.kill();
  }

  virtual int start(const std::vector<std::string> &cmd) final;
  virtual int stop(unsigned int timeout) final;
	virtual bool is_running() final;
};
