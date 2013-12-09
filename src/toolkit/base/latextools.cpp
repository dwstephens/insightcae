/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2013  hannes <email>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "latextools.h"

#include <string>

#include "boost/algorithm/string.hpp"

namespace insight
{

std::string cleanSymbols(const std::string& s)
{
  std::string result(s);
  boost::replace_all(result, "_", "\\_");
  boost::replace_all(result, "#", "\\#");
  boost::replace_all(result, "^", "\\textasciicircum");
  boost::replace_all(result, "[", "{[}");
  boost::replace_all(result, "]", "{]}");
  return result;
}

}