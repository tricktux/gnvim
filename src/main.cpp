/// @file main.cpp
/// @brief Main for gnvim
/// @author Reinaldo Molina
/// @version  0.0
/// @date Mar 05 2019
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
// along with this program.  If not, see <http:www.gnu.org/licenses/>.

// #include "application.hpp"
// #include "libnvc.hpp"
#include <cinttypes>
#include <iostream>
#include <stdexcept>
#include <string>

#include "easylogging++.h"
#include "rpc/iodevice.hpp"
#include "rpc/msgpack.hpp"
#include "nvimapi.hpp"

INITIALIZE_EASYLOGGINGPP

int main(int argc, char **argv) {
  START_EASYLOGGINGPP(argc, argv);

	// libnvc::reproc_device reproc_dev;
	// reproc_dev.spawn();

	// libnvc::api_client client(&reproc_dev);
	// client.nvim_ui_attach(100, 80, {{"rgb", true}, {"ext_linegrid", true}});
	// client.nvim_input("$i123<CR>123<ESC>");
	// client.nvim_buf_set_name(1, "1234");
	// while (1) {}
	int timeout = 10;
	nvimrpc::ReprocDevice device;
	std::vector<const char *> args{{"nvim", "-u", "NORC", "--embed", nullptr}};
	device.spawn(args, timeout);

	nvimrpc::NvimApi api{device};
	api.nvim_ui_attach(80, 60, {{"rgb", true}});
	// api.nvim_input("$i123<CR>123<ESC>");
	nvimrpc::Object obj = api.nvim_buf_get_var(0, "current_syntax");
	nvimrpc::ObjectWrapper wrap{obj};
	device.kill();
}
