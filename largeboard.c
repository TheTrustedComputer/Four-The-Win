#include "largeboard.h"

LargePosition LargePosition_add(const LargePosition left, const LargePosition right) {
	return (LargePosition) { left.value[0] + right.value[0], left.value[1] + right.value[1] + (left.value[0] + right.value[0] < left.value[0]) };
}

LargePosition LargePosition_subtract(const LargePosition left, const LargePosition right) {
	return (LargePosition) { left.value[0] - right.value[0], left.value[1] - right.value[1] - (left.value[0] - right.value[0] > left.value[0]) };
}

LargePosition LargePosition_multiply(const LargePosition left, const LargePosition right) {
	uint_fast64_t leftBits[4] = { left.value[1] >> 32, left.value[1] & 0xffffffff, left.value[0] >> 32, left.value[0] & 0xffffffff };
	uint_fast64_t rightBits[4] = { right.value[1] >> 32, right.value[1] & 0xffffffff, right.value[0] >> 32, right.value[0] & 0xffffffff };
	uint_fast64_t products[4][4], firstProduct, secondProduct, thirdProduct, fourthProduct; int i, j;
	for (i = 3; i >= 0; --i) {
		for (j = 3; j >= 0; --j) {
			products[3 - j][i] = leftBits[j] * rightBits[i];
		}
	}
	firstProduct = (products[0][0] & 0xffffffff) + (products[0][1] >> 32);
	firstProduct += (products[1][1] & 0xffffffff) + (products[1][2] >> 32);
	firstProduct += (products[2][2] & 0xffffffff) + (products[2][3] >> 32);
	firstProduct += products[3][3] & 0xffffffff;
	secondProduct = (products[0][1] & 0xffffffff) + (products[0][2] >> 32);
	secondProduct += (products[1][2] & 0xffffffff) + (products[1][3] >> 32);
	secondProduct += products[2][3] & 0xffffffff;
	thirdProduct = (products[0][2] & 0xffffffff) + (products[0][3] >> 32);
	thirdProduct += (products[1][3] & 0xffffffff);
	fourthProduct = products[0][3] & 0xffffffff;
	firstProduct += secondProduct >> 32;
	secondProduct += thirdProduct >> 32;
	thirdProduct += fourthProduct >> 32;
	firstProduct &= 0xffffffff;
	secondProduct &= 0xffffffff;
	thirdProduct &= 0xffffffff;
	fourthProduct &= 0xffffffff;
	return (LargePosition) { (thirdProduct << 32) | fourthProduct, (firstProduct << 32) | secondProduct};
}

LargePosition LargePosition_divide(const LargePosition left, const LargePosition right) {
	if (right.value[0] || right.value[1]) {
		if (right.value[0] == 1 && !right.value[1]) {
			return left;
		}
		if (left.value[0] == right.value[0] && left.value[1] == right.value[1]) {
			return (LargePosition) { 1, 0 };
		}
		if (!(left.value[0] || left.value[1])) {
			return (LargePosition) { 0, 0 };
		}
		LargePosition divmod[2] = { {0,0},{0,0} }, evaluator[2];
		for (unsigned x = LargePosition_countBits(left) + 1; --x;) {
			divmod[0] = LargePosition_leftShift(divmod[0], (LargePosition){1, 0});
			divmod[1] = LargePosition_leftShift(divmod[1], (LargePosition){1, 0});
			evaluator[0] = LargePosition_rightShift(left, (LargePosition){x - 1, 0});
			evaluator[1] = LargePosition_bitwise_and(evaluator[0], (LargePosition){1, 0});
			if (evaluator[1].value[0] || evaluator[1].value[1]) {
				LargePosition_preIncrement(&divmod[1]);
			}
			if (LargePosition_bool_greaterThanOrEqual(divmod[1], right)) {
				divmod[1] = LargePosition_subtract(divmod[1], right);
				LargePosition_preIncrement(&divmod[0]);
			}
		}
		return divmod[0];
	}
	else {
		fprintf(stderr, "The right side of LargePosition was zero and thus attempted to divide by zero.\n");
		return (LargePosition) { -1ll, -1ll };
	}
}

void LargePosition_preIncrement(LargePosition *lb) {
	*lb = LargePosition_add(*lb, (LargePosition){1, 0});
}

LargePosition LargePosition_postIncrement(LargePosition *lb) {
	LargePosition oldBoard = *lb;
	*lb = LargePosition_add(*lb, (LargePosition){1, 0});
	return oldBoard;
}

void LargePosition_preDecrement(LargePosition *lb) {
	*lb = LargePosition_subtract(*lb, (LargePosition){1, 0});
}

LargePosition LargePosition_postDecrement(LargePosition *lb) {
	LargePosition oldBoard = *lb;
	*lb = LargePosition_subtract(*lb, (LargePosition){1, 0});
	return oldBoard;
}

LargePosition LargePosition_bitwise_and(const LargePosition left, const LargePosition right) {
	return (LargePosition) { left.value[0] & right.value[0], left.value[1] & right.value[1] };
}

LargePosition LargePosition_bitwise_or(const LargePosition left, const LargePosition right) {
	return (LargePosition) { left.value[0] | right.value[0], left.value[1] | right.value[1] };
}

LargePosition LargePosition_bitwise_xor(const LargePosition left, const LargePosition right) {
	return (LargePosition) { left.value[0] ^ right.value[0], left.value[1] ^ right.value[1] };
}

LargePosition LargePosition_bitwise_not(const LargePosition lb) {
	return (LargePosition) { ~lb.value[0], ~lb.value[1] };
}

LargePosition LargePosition_leftShift(const LargePosition left, const LargePosition right) {
	const uint_fast64_t SHIFTER = right.value[0];
	if (SHIFTER >= 128 || right.value[1]) {
		return (LargePosition) { 0, 0 };
	}
	if (!SHIFTER) {
		return left;
	}
	if (SHIFTER == 64) {
		return (LargePosition) { 0, left.value[0] };
	}
	if (SHIFTER < 64) {
		return (LargePosition) { left.value[0] << SHIFTER, (left.value[1] << SHIFTER) + (left.value[0] >> (64 - SHIFTER)) };
	}
	if (SHIFTER > 64) {
		return (LargePosition) { 0, left.value[0] << (SHIFTER - 64) };
	}
	return (LargePosition) { 0, 0 };
}

LargePosition LargePosition_rightShift(const LargePosition left, const LargePosition right) {
	const uint_fast64_t SHIFTER = right.value[0];
	if (SHIFTER >= 128 || right.value[1]) {
		return (LargePosition) { 0, 0 };
	}
	if (!SHIFTER) {
		return left;
	}
	if (SHIFTER == 64) {
		return (LargePosition) {left.value[1], 0 };
	}
	if (SHIFTER < 64) {
		return (LargePosition) { (left.value[1] << (64 - SHIFTER)) + (left.value[0] >> SHIFTER), left.value[1] >> SHIFTER };
	}
	if (SHIFTER > 64) {
		return (LargePosition) { left.value[1] >> (SHIFTER - 64), 0 };
	}
	return (LargePosition) { 0, 0 };
}

unsigned LargePosition_countBits(const LargePosition lb) {
	uint_fast64_t highBits, lowBits;
	unsigned bits = 0;
	if (lb.value[1]) {
		bits = 64;
		for (highBits = lb.value[1]; highBits; highBits >>= 1) {
			++bits;
		}
	}
	else {
		for (lowBits = lb.value[0]; lowBits; lowBits >>= 1) {
			++bits;
		}
	}
	return bits;
}

bool LargePosition_bool(const LargePosition lb) {
	return lb.value[0] || lb.value[1];
}

bool LargePosition_bool_equal(const LargePosition left, const LargePosition right) {
	return (left.value[0] == right.value[0]) && (left.value[1] == right.value[1]);
}

bool LargePosition_bool_lessThan(const LargePosition left, const LargePosition right) {
	return (left.value[1] == right.value[1]) ? left.value[0] < right.value[0] : left.value[1] < right.value[1];
}

bool LargePosition_bool_greaterThan(const LargePosition left, const LargePosition right) {
	return (left.value[1] == right.value[1]) ? left.value[0] > right.value[0] : left.value[1] > right.value[1];
}

bool LargePosition_bool_lessThanOrEqual(const LargePosition left, const LargePosition right) {
	return LargePosition_bool_lessThan(left, right) || LargePosition_bool_equal(left, right);
}

bool LargePosition_bool_greaterThanOrEqual(const LargePosition left, const LargePosition right) {
	return LargePosition_bool_greaterThan(left, right) || LargePosition_bool_equal(left, right);
}