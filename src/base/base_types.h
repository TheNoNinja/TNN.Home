#ifndef BASE_TYPES_H
#define BASE_TYPES_H

#include <stdint.h>

#define UNUSED(x) ((void)(x))

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef float f32;
typedef double f64;

typedef u8 b8;
typedef u32 b32;

#define internal static

typedef struct str str;
struct str
{
    u8 *data;
    u64 length;
};
#define STR_LITERAL(s) ((str){ (u8 *)(s), sizeof(s) - 1 })

#endif
