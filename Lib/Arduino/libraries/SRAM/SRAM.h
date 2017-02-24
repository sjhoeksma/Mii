/**
 * This is a header contains static functions used to determin if
 * variable is in shared memory of in flash memory
 *
 *******************************************************************************
    Copyright (C) 2012 S.J.Hoeksma


    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************
 */
#ifndef SRAM_h
#define SRAM_h

#include <Arduino.h>

int freeMemory();
byte inSRAM(const void *s);
#endif