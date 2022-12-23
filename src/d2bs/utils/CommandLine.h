/*
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.

*/

#pragma once

#include <map>
#include <string>
#include <vector>

class CommandLine {
 public:
  CommandLine(std::string cmdline);
  ~CommandLine() noexcept = default;

  bool contains(const std::string& arg);
  std::string value(const std::string& arg);
  std::map<std::string, std::string> args();

 private:
  void trim_binary_path(std::string& cmdline);
  std::string& trim_quote(std::string& val);
  std::vector<std::string> parse_args(std::string& cmdline);

 private:
  std::map<std::string, std::string> args_;
};
