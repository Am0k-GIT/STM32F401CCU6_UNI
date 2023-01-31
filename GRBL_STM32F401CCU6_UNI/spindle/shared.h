/*

  shared.h - shared spindle plugin symbols

  Part of grblHAL

  Copyright (c) 2022 Terje Io

  Grbl is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Grbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.

*/

#pragma once

#ifdef ARDUINO
#include "../driver.h"
#else
#include "driver.h"
#endif

#define SPINDLE_ALL        -1
#define SPINDLE_HUANYANG1   1
#define SPINDLE_HUANYANG2   2
#define SPINDLE_GS20        3
#define SPINDLE_YL620A      4
#define SPINDLE_MODVFD      5
#define SPINDLE_H100        6 // Not tested

/**/
