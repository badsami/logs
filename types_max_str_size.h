#pragma once


///////////////////////////////////////////////////////////////////////////////////////////////////
//// Binary
#define U8_MAX_BIN_STR_SIZE  8
#define U16_MAX_BIN_STR_SIZE 16
#define U32_MAX_BIN_STR_SIZE 32
#define U64_MAX_BIN_STR_SIZE 64

#define S8_MAX_BIN_STR_SIZE  8
#define S16_MAX_BIN_STR_SIZE 16
#define S32_MAX_BIN_STR_SIZE 32
#define S64_MAX_BIN_STR_SIZE 64

#define F32_MAX_BIN_STR_SIZE 32


///////////////////////////////////////////////////////////////////////////////////////////////////
//// Decimal
#define U8_MAX_DEC_STR_SIZE  3
#define U16_MAX_DEC_STR_SIZE 5
#define U32_MAX_DEC_STR_SIZE 10
#define U64_MAX_DEC_STR_SIZE 20

#define S8_MAX_DEC_STR_SIZE  4
#define S16_MAX_DEC_STR_SIZE 6
#define S32_MAX_DEC_STR_SIZE 11
#define S64_MAX_DEC_STR_SIZE 20

#define F32_DEC_FRAC_DEFAULT_STR_SIZE 6
#define F32_DEC_FRAC_MAX_STR_SIZE     9

// Should be a 1 followed by F32_DEC_FRAC_DEFAULT_STR_SIZE zeros
#define F32_DEC_FRAC_MULT 1000000.f

// Sign + integer part + period + fractional part
#define F32_MAX_DEC_STR_SIZE (1 + U32_MAX_DEC_STR_SIZE + 1 + F32_DEC_FRAC_MAX_STR_SIZE)


///////////////////////////////////////////////////////////////////////////////////////////////////
//// Hexadecimal
#define U8_MAX_HEX_STR_SIZE  2
#define U16_MAX_HEX_STR_SIZE 4
#define U32_MAX_HEX_STR_SIZE 8
#define U64_MAX_HEX_STR_SIZE 16

#define S8_MAX_HEX_STR_SIZE  2
#define S16_MAX_HEX_STR_SIZE 4
#define S32_MAX_HEX_STR_SIZE 8
#define S64_MAX_HEX_STR_SIZE 16

#define F32_MAX_HEX_STR_SIZE 8
