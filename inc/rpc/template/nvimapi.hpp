/// @file nvimapi.hpp
/// @brief High level controller of nvimrpc
/// @author Reinaldo Molina
/// @version  0.0
/// @date Dec 31 2019
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

#ifndef NVIMAPI_HPP
#define NVIMAPI_HPP

#include "rpc/iodevice.hpp"

#include <queue>
#include <string>
#include <vector>

namespace nvimrpc {

class NvimApi {
  size_t msgid;
  std::queue<std::string> pending_notif;
	IoDevice &device;

	template <typename... Params>
	size_t dispatch(std::string_view func, Params&& ... params);
	template<typename T> auto poll(size_t msgid, size_t timeout);
	size_t get_new_msgid() { return ++msgid; }

public:
	explicit NvimApi(IoDevice &_device) : msgid(0), device(_device) {}
  ~NvimApi() = default;

	// Generated apis
	// clang-format off
	{% for req in nvim_reqs %}
		{{req.return_type}} {{req.name}}({% for arg in req.args %}{{arg.type}} {{arg.name}}{% if not loop.last %}, {% endif %}{% endfor %}) {
			{% if req.return_type != 'void' %} {
				const size_t msgid = dispatch("{{req.name}}", {{ req.parameters|join(", ", attribute='name') }});
				return poll<{{req.return_type.native_type}}>(msgid, 100);
			 }
			{% endif %}
			const size_t msgid = dispatch("{{req.name}}");
			return poll<>(msgid, 100);
		}
	{% endfor %}
	// clang-format on
};

} // namespace nvimrpc
#endif
