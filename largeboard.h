/*
    The MIT License (MIT)

    Copyright (c) 2013 - 2017 Jason Lee @ calccrypto at gmail.com

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.
*/

/*
    Copyright (C) 2020 TheTrustedComputer
*/

#ifndef LARGEBOARD_H
#define LARGEBOARD_H

typedef struct {
	uint_fast64_t value[2];
} LargePosition;

typedef struct {
	LargePosition value[2];
} HugePosition;

LargePosition LargePosition_add(const LargePosition, const LargePosition);
LargePosition LargePosition_subtract(const LargePosition, const LargePosition);
LargePosition LargePosition_multiply(const LargePosition, const LargePosition);
LargePosition LargePosition_divide(const LargePosition, const LargePosition);

void LargePosition_preIncrement(LargePosition*);
LargePosition LargePosition_postIncrement(LargePosition*);
void LargePosition_preDecrement(LargePosition*);
LargePosition LargePosition_postDecrement(LargePosition*);

LargePosition LargePosition_bitwise_and(const LargePosition, const LargePosition);
LargePosition LargePosition_bitwise_or(const LargePosition, const LargePosition);
LargePosition LargePosition_bitwise_xor(const LargePosition, const LargePosition);
LargePosition LargePosition_bitwise_not(const LargePosition);
LargePosition LargePosition_leftShift(const LargePosition, const LargePosition);
LargePosition LargePosition_rightShift(const LargePosition, const LargePosition);

unsigned LargePosition_countBits(const LargePosition);

bool LargePosition_bool(const LargePosition);
bool LargePosition_bool_equal(const LargePosition, const LargePosition);
bool LargePosition_bool_lessThan(const LargePosition, const LargePosition);
bool LargePosition_bool_greaterThan(const LargePosition, const LargePosition);
bool LargePosition_bool_lessThanOrEqual(const LargePosition, const LargePosition);
bool LargePosition_bool_greaterThanOrEqual(const LargePosition, const LargePosition);

#endif
