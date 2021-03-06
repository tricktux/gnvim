/// @file test_cli_args.cpp
/// @brief Tests for cli args interface
/// @author Reinaldo Molina
/// @version  0.0
/// @date Apr 16 2019
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

#include "cli_args.hpp"
#include "cxxopts.hpp"
#include "easylogging/easylogging++.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>

INITIALIZE_EASYLOGGINGPP

TEST(cxxopts_args, loading) {
  const char *argv[] = {"gnvim",       "-n",      "/usr/bin/nvim",
                        "--maximized", "--help",  "--timeout=13",
                        "file2.cpp",   "filee3.h"};

  int argc = sizeof(argv) / sizeof(char *);
  char **argv_ = new char *[argc];
  for (int k = 0; k < argc; k++) {
    argv_[k] = new char[32];
    std::strncpy(argv_[k], argv[k], 32);
  }

  cli::Options opt;
  {
    cli::CliArgs args(cli::PROGRAM_NAME, cli::PROGRAM_DESCRIPTION);

    args.add_options(opt.bool_args);
    args.add_options(opt.str_args);
    args.add_options(opt.int_args);
    args.add_pos_options(opt.pos_arg);

    ASSERT_EQ(args.parse(argc, argv_), 0);

    std::cout << args.get_help() << std::endl;
  }
  std::string_view nvim = opt.get_arg("n,nvim", std::string_view());
  ASSERT_EQ(argv[2], nvim);
  const std::vector<std::string> &pos = opt.get_pos_arg();
  ASSERT_EQ(pos.size(), 2);
  for (int k = 6; k < argc; k++) {
    ASSERT_EQ(argv[k], pos[k - 6]);
  }
  bool max = opt.get_arg("m,maximized", false);
  ASSERT_EQ(max, true);
  int t = opt.get_arg("t,timeout", 0);
  ASSERT_EQ(t, 13);
  bool h = opt.get_arg("h,help", false);
  ASSERT_EQ(h, true);

  for (int k = 0; k < argc; k++) {
    delete[] argv_[k];
  }
  delete[] argv_;
}

TEST(cxxopts_args, no_args) {
  const char *argv[] = {"gnvim"};

  int argc = sizeof(argv) / sizeof(char *);
  char **argv_ = new char *[argc];
  for (int k = 0; k < argc; k++) {
    argv_[k] = new char[32];
    std::strncpy(argv_[k], argv[k], 32);
  }

  cli::Options opt;
  {
    cli::CliArgs args(cli::PROGRAM_NAME, cli::PROGRAM_DESCRIPTION);

    args.add_options(opt.bool_args);
    args.add_options(opt.str_args);
    args.add_options(opt.int_args);
    args.add_pos_options(opt.pos_arg);

    ASSERT_EQ(args.parse(argc, argv_), 0);
  }

  bool max = opt.get_arg("m,maximized", false);
  ASSERT_EQ(max, false);
  int t = opt.get_arg("t,timeout", 0);
  ASSERT_EQ(t, 15);
  bool h = opt.get_arg("h,help", false);
  ASSERT_EQ(h, false);

  for (int k = 0; k < argc; k++) {
    delete[] argv_[k];
  }
  delete[] argv_;
}

TEST(cxxopts_args, nvim_args) {
  const char *argv[] = {"gnvim",
                        "-n",
                        "/usr/bin/nvim",
                        "--maximized",
                        "--help",
                        "--timeout=13",
                        "-a",
												"\"set termguicolors | set background=light\"",
                        "file2.cpp",
                        "filee3.h"};

  int argc = sizeof(argv) / sizeof(char *);
  char **argv_ = new char *[argc];
  for (int k = 0; k < argc; k++) {
    argv_[k] = new char[32];
    std::strncpy(argv_[k], argv[k], 32);
  }

  cli::Options opt;
  {
    cli::CliArgs args(cli::PROGRAM_NAME, cli::PROGRAM_DESCRIPTION);

    args.add_options(opt.bool_args);
    args.add_options(opt.str_args);
    args.add_options(opt.int_args);
    args.add_pos_options(opt.pos_arg);

    ASSERT_EQ(args.parse(argc, argv_), 0);
  }
	std::string_view nvim = opt.get_arg("n,nvim", std::string_view());
	ASSERT_EQ(argv[2], nvim);
	const std::vector<std::string> &pos = opt.get_pos_arg();
	ASSERT_EQ(pos.size(), 2);
	for (int k = 8; k < argc; k++) {
		ASSERT_EQ(argv[k], pos[k - 8]);
	}
	bool max = opt.get_arg("m,maximized", false);
	ASSERT_EQ(max, true);
	int t = opt.get_arg("t,timeout", 0);
	ASSERT_EQ(t, 13);
	bool h = opt.get_arg("h,help", false);
	ASSERT_EQ(h, true);

	std::string_view nargs = opt.get_arg("a,nvim_args", std::string_view());
	ASSERT_EQ(argv[7], nargs);

  for (int k = 0; k < argc; k++) {
    delete[] argv_[k];
  }
  delete[] argv_;
}
