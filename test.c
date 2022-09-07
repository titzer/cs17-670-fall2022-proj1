#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "common.h"
#include "test.h"

typedef struct {
  const char* name;
  int (*run)();
} test_t;

#define VERBOSE 0

#define CHECK_EQ(x, y) do {			\
    int vx = (x);						\
    if (VERBOSE) printf("  CHECK(%d == %s)\n", vx, #y);		\
    if (vx != (y)) {						\
      return 0;							\
    }								\
  } while(0)

// Test macros
#define BYTES(...) const uint8_t bytes[] = { __VA_ARGS__ }
// Polymorphic macro to check for 
#define OK_poly(type, fmt, decode, len, x) do {				\
    ssize_t gotlen = 0;							\
    type val = (x);							\
    type gotval = decode(bytes, bytes + sizeof(bytes), &gotlen);	\
    if (gotlen < 0) {							\
      printf("expected %d, but failed with %zd @ %s:%d\n", len, gotlen, __FILE__, __LINE__); \
      return 0;								\
    }									\
    if (gotval != val) {						\
      printf("expected " fmt ", but got " fmt " @ %s:%d\n", val, gotval, __FILE__, __LINE__); \
      return 0;								\
    }									\
  } while(0) 

#define OK_i32(len, val) OK_poly(int32_t, "%d", decode_i32leb, len, val);
#define OK_u32(len, val) OK_poly(uint32_t, "%u", decode_u32leb, len, val);
#define OK_i64(len, val) OK_poly(int64_t, "%" PRId64, decode_i64leb, len, val);
#define OK_u64(len, val) OK_poly(uint64_t, "%" PRIu64, decode_u64leb, len, val);

#define ERR_poly(type, fmt, decode) do {				\
    ssize_t gotlen = 0;							\
    type gotval = decode(bytes, bytes + sizeof(bytes), &gotlen);	\
    if (gotlen > 0) {							\
      printf("expected fail, but got val=" fmt ", len=%zd @ %s:%d\n", gotval, gotlen, __FILE__, __LINE__); \
      return 0;								\
    }									\
  } while(0)

#define ERR_i32 ERR_poly(int32_t, "%d", decode_i32leb);
#define ERR_u32 ERR_poly(uint32_t, "%u", decode_u32leb);
#define ERR_i64 ERR_poly(int64_t, "%" PRId64, decode_i64leb);
#define ERR_u64 ERR_poly(uint64_t, "%" PRIu64, decode_u64leb);

//================================================================================
// Tests and test list
int test_i32() {
  { BYTES(0x00); OK_i32(1, 0); }
  { BYTES(0x01); OK_i32(1, 1); }
  { BYTES(0x0D); OK_i32(1, 13); }
  { BYTES(0x70); OK_i32(1, -16); }
  { BYTES(0x0F, 0x00); OK_i32(1, 15); }
  { BYTES(0x1F, 0x06); OK_i32(1, 31); }

  { BYTES(0x74); OK_i32(1, -12); }
  { BYTES(0xF3, 0x7F); OK_i32(2, -13); }
  { BYTES(0xF1, 0xFF, 0x7F); OK_i32(3, -15); }

  { BYTES(0xA7, 0x7F); OK_i32(2, -89); }
  { BYTES(0xA7, 0xFF, 0x7F); OK_i32(3, -89); }
  { BYTES(0xA7, 0xFF, 0xFF, 0x7F); OK_i32(4, -89); }
  { BYTES(0xA7, 0xFF, 0xFF, 0x7F); OK_i32(4, -89); }

  { BYTES(0x80, 0x01); OK_i32(2, 128); }
  { BYTES(0x83, 0x80, 0x03); OK_i32(3, 49155); }
  { BYTES(0x83, 0x80, 0x83, 0x00); OK_i32(4, 49155); }
  { BYTES(0x83, 0x80, 0x83, 0x80, 0x00); OK_i32(5, 49155); }

  { BYTES(0x80); ERR_i32; }
  { BYTES(0x8E, 0x8E); ERR_i32; }
  { BYTES(0x9E, 0x9F, 0x99); ERR_i32; }
  { BYTES(0xFF, 0xFF, 0xFF, 0xFF); ERR_i32; }

  { BYTES(0x21); OK_i32(1, 33); }
  { BYTES(0xE7, 0x01); OK_i32(2, 231); }
  { BYTES(0xD1, 0x0C); OK_i32(2, 1617); }
  { BYTES(0xB7, 0xD8, 0x00); OK_i32(3, 11319); }
  { BYTES(0x81, 0xEB, 0x04); OK_i32(3, 79233); }
  { BYTES(0x87, 0xED, 0x21); OK_i32(3, 554631); }
  { BYTES(0xB1, 0xFB, 0xEC, 0x01); OK_i32(4, 3882417); }
  { BYTES(0xD7, 0xDF, 0xFA, 0x0C); OK_i32(4, 27176919); }
  { BYTES(0xE1, 0x9D, 0xDB, 0xDA, 0x00); OK_i32(5, 190238433); }

    { BYTES(0x61); OK_i32(1, -31); }
  { BYTES(0xA7, 0x7E); OK_i32(2, -217); }
  { BYTES(0x91, 0x74); OK_i32(2, -1519); }
  { BYTES(0xF7, 0xAC, 0x7F); OK_i32(3, -10633); }
  { BYTES(0xC1, 0xBA, 0x7B); OK_i32(3, -74431); }
  { BYTES(0xC7, 0x99, 0x60); OK_i32(3, -521017); }
  { BYTES(0xF1, 0xB2, 0xA1, 0x7E); OK_i32(4, -3647119); }
  { BYTES(0x97, 0xE4, 0xE9, 0x73); OK_i32(4, -25529833); }
  { BYTES(0xA1, 0xBD, 0xE4, 0xAA, 0x7F); OK_i32(5, -178708831); }

  return 1;
}

int test_i32ext() {

  { BYTES(0x80, 0x80, 0x80, 0x80, 0x7f); OK_i32(5, -268435456); }
  
  { BYTES(0x81, 0x84, 0x87, 0x8a, 0x08); ERR_i32; }
  { BYTES(0x82, 0x85, 0x88, 0x8b, 0x09); ERR_i32; }
  { BYTES(0x83, 0x86, 0x89, 0x8c, 0x10); ERR_i32; }
  { BYTES(0x8a, 0x8d, 0x89, 0x86, 0x20); ERR_i32; }
  { BYTES(0x8b, 0x8e, 0x88, 0x85, 0x40); ERR_i32; }
  { BYTES(0x8b, 0x8e, 0x88, 0x85, 0x70); ERR_i32; }
  { BYTES(0x8b, 0x8e, 0x88, 0x85, 0x71); ERR_i32; }
  { BYTES(0x8c, 0x8f, 0x87, 0x84, 0x6e); ERR_i32; }

  { BYTES(0x83, 0x80, 0x83, 0x80, 0x80, 0x00); ERR_i32; }
  { BYTES(0xA7, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F); ERR_i32; }
  { BYTES(0x83, 0x80, 0x83, 0x80, 0x80, 0x01); ERR_i32; }
  { BYTES(0xA7, 0xFF, 0xFF, 0xFF, 0xFF, 0x7E); ERR_i32; }
  return 1;
}

int test_u32() {
  { BYTES(0x70); OK_u32(1, 112); }
  { BYTES(0xA7, 0x7F); OK_u32(2, 16295); }
  { BYTES(0xA7, 0xFE, 0x7F); OK_u32(3, 2096935); }
  { BYTES(0xA7, 0xFE, 0xFD, 0x7F); OK_u32(4, 268402471); }
  { BYTES(0xA7, 0xF0, 0xF1, 0xF2, 0x0E); OK_u32(5, 3999037479u); }

  { BYTES(0x1D); OK_u32(1, 29); }
  { BYTES(0xCB, 0x01); OK_u32(2, 203); }
  { BYTES(0x8D, 0x0B); OK_u32(2, 1421); }
  { BYTES(0xDB, 0x4D); OK_u32(2, 9947); }
  { BYTES(0xFD, 0x9F, 0x04); OK_u32(3, 69629); }
  { BYTES(0xEB, 0xDF, 0x1D); OK_u32(3, 487403); }
  { BYTES(0xED, 0x9E, 0xD0, 0x01); OK_u32(4, 3411821); }
  { BYTES(0xFB, 0xD7, 0xB1, 0x0B); OK_u32(4, 23882747); }
  { BYTES(0xDD, 0xE7, 0xDB, 0x4F); OK_u32(4, 167179229); }
  { BYTES(0x8B, 0xD6, 0x82, 0xAE, 0x04); OK_u32(5, 1170254603); }
  { BYTES(0xCD, 0xDA, 0x92, 0xC2, 0x0E); OK_u32(5, 3896814925); }
  { BYTES(0x9B, 0xFA, 0x82, 0xCF, 0x05); OK_u32(5, 1507900699); }
  { BYTES(0xBD, 0xD7, 0x94, 0xA9, 0x07); OK_u32(5, 1965370301); }
  { BYTES(0xAB, 0xE4, 0x90, 0xA0, 0x03); OK_u32(5, 872690219); }
  { BYTES(0xAD, 0xBE, 0xF5, 0xE0, 0x06); OK_u32(5, 1813864237); }
  
  { BYTES(0xA7, 0xF0, 0xF1, 0xF2, 0x1E); ERR_u32; }
  { BYTES(0xA7, 0xF0, 0xF1, 0xF2, 0xFE); ERR_u32; }

  return 1;
}

int test_u32ext() {
  { BYTES(0x80, 0x80, 0x80, 0x80, 0x00); OK_u32(5, 0); }
  { BYTES(0x80, 0x80, 0x80, 0x80, 0x01); OK_u32(5, 0x10000000); }
  { BYTES(0x80, 0x80, 0x80, 0x80, 0x07); OK_u32(5, 0x70000000); }
  { BYTES(0x80, 0x80, 0x80, 0x80, 0x0d); OK_u32(5, 0xd0000000); }
  { BYTES(0x80, 0x80, 0x80, 0x80, 0x0f); OK_u32(5, 0xf0000000); }
	
  { BYTES(0x83, 0x86, 0x89, 0x8c, 0x10); ERR_u32; }
  { BYTES(0x8a, 0x8d, 0x89, 0x86, 0x20); ERR_u32; }
  { BYTES(0x8b, 0x8e, 0x88, 0x85, 0x40); ERR_u32; }
  { BYTES(0x8b, 0x8e, 0x88, 0x85, 0x70); ERR_u32; }
  { BYTES(0x8c, 0x8f, 0x87, 0x84, 0x7e); ERR_u32; }

  { BYTES(0xA7, 0xF0, 0xF1, 0xF2, 0x8E, 0x00); ERR_u32; }
  { BYTES(0xA7, 0xF0, 0xF1, 0xF2, 0x80, 0x00); ERR_u32; }
  { BYTES(0xA7, 0xF0, 0xF1, 0xF2, 0x80, 0x01); ERR_u32; }
  return 1;
}

int test_i64() {
  { BYTES(0x00); OK_i64(1, 0); }
  { BYTES(0x01); OK_i64(1, 1); }
  { BYTES(0x0D); OK_i64(1, 13); }
  { BYTES(0x70); OK_i64(1, -16); }

  { BYTES(0xA7, 0x7F); OK_i64(2, -89); }
  { BYTES(0xA6, 0xFF, 0x7F); OK_i64(3, -90); }
  { BYTES(0xA5, 0xFF, 0xFF, 0x7F); OK_i64(4, -91); }
  { BYTES(0xA4, 0xFF, 0xFF, 0x7F); OK_i64(4, -92); }
  { BYTES(0xA3, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F); OK_i64(6, -93); }
  { BYTES(0xA3, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F); OK_i64(7, -93); }
  { BYTES(0xA3, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F); OK_i64(8, -93); }
  { BYTES(0xA3, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F); OK_i64(9, -93); }
  { BYTES(0xA3, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F); OK_i64(10, -93); }

  { BYTES(0xF4, 0xD2, 0xA6, 0x87, 0x01); OK_i64(5, 283748724); }

  { BYTES(0x81, 0x81, 0x81, 0x81, 0x01); OK_i64(5, 270549121); }
  { BYTES(0x81, 0x82, 0x83, 0x84, 0x05); OK_i64(5, 1350615297); }
  { BYTES(0x81, 0x82, 0x83, 0x84, 0x85, 0x06); OK_i64(6, 207509045505); }
  { BYTES(0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x07); OK_i64(7, 30993834623233); }
  { BYTES(0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x08); OK_i64(8, 4534593461993729); }
  { BYTES(0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x09); OK_i64(9, 653052939803345153); }

  { BYTES(0x63); OK_i64(1, -29); }
  { BYTES(0xB5, 0x7E); OK_i64(2, -203); }
  { BYTES(0xF3, 0x74); OK_i64(2, -1421); }
  { BYTES(0xA5, 0xB2, 0x7F); OK_i64(3, -9947); }
  { BYTES(0x83, 0xE0, 0x7B); OK_i64(3, -69629); }
  { BYTES(0x95, 0xA0, 0x62); OK_i64(3, -487403); }
  { BYTES(0x93, 0xE1, 0xAF, 0x7E); OK_i64(4, -3411821); }
  { BYTES(0x85, 0xA8, 0xCE, 0x74); OK_i64(4, -23882747); }
  { BYTES(0xA3, 0x98, 0xA4, 0xB0, 0x7F); OK_i64(5, -167179229); }
  { BYTES(0xF5, 0xA9, 0xFD, 0xD1, 0x7B); OK_i64(5, -1170254603); }
  { BYTES(0xB3, 0xA5, 0xED, 0xBD, 0x61); OK_i64(5, -8191782221); }
  { BYTES(0xE5, 0x85, 0xFD, 0xB0, 0xAA, 0x7E); OK_i64(6, -57342475547); }
  { BYTES(0xC3, 0xA8, 0xEB, 0xD6, 0xA8, 0x74); OK_i64(6, -401397328829); }
  { BYTES(0xD5, 0x9B, 0xEF, 0xDF, 0x9C, 0xAE, 0x7F); OK_i64(7, -2809781301803); }
  { BYTES(0xD3, 0xC1, 0x8A, 0x9F, 0xC9, 0xC3, 0x7B); OK_i64(7, -19668469112621); }
  { BYTES(0xC5, 0xCB, 0xC9, 0xD9, 0x80, 0xD9, 0x60); OK_i64(7, -137679283788347); }
  { BYTES(0xE3, 0x90, 0x83, 0xF3, 0x84, 0xEF, 0xA4, 0x7E); OK_i64(8, -963754986518429); }
  { BYTES(0xB5, 0xF5, 0x95, 0xA5, 0xA2, 0x89, 0x82, 0x74); OK_i64(8, -6746284905629003); }

  
  { BYTES(0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x08); ERR_i64; }
  { BYTES(0xA3, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x6F); ERR_i64; }

  return 1;
}

int test_i64ext() {
  { BYTES(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x7f); OK_i64(10, 0x8000000000000000L); }
  { BYTES(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00); OK_i64(10, 0); }

  { BYTES(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01); ERR_i64; }
  { BYTES(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x02); ERR_i64; }
  { BYTES(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x04); ERR_i64; }
  { BYTES(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x08); ERR_i64; }
  { BYTES(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x10); ERR_i64; }
  { BYTES(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x20); ERR_i64; }
  { BYTES(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x40); ERR_i64; }

  { BYTES(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80); ERR_i64; }
  { BYTES(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x50); ERR_i64; }
  { BYTES(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x20); ERR_i64; }
  { BYTES(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x30); ERR_i64; }

  { BYTES(0xA3, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F); ERR_i64; }
  { BYTES(0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x80, 0x01); ERR_i64; }
  { BYTES(0xA3, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7E); ERR_i64; }
	
  return 1;
}

int test_u64() {
  { BYTES(0x00); OK_u64(1, 0); }
  { BYTES(0x01); OK_u64(1, 1); }
  { BYTES(0x0D); OK_u64(1, 13); }
  { BYTES(0x70); OK_u64(1, 112); }

  { BYTES(0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x09); OK_u64(9, 653052939803345153); }
  { BYTES(0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x00); OK_u64(10, 653052939803345153); }

  { BYTES(0x1A); OK_u64(1, 26); }
  { BYTES(0xB6, 0x01); OK_u64(2, 182); }
  { BYTES(0xFA, 0x09); OK_u64(2, 1274); }
  { BYTES(0xD6, 0x45); OK_u64(2, 8918); }
  { BYTES(0xDA, 0xE7, 0x03); OK_u64(3, 62426); }
  { BYTES(0xF6, 0xD5, 0x1A); OK_u64(3, 436982); }
  { BYTES(0xBA, 0xD9, 0xBA, 0x01); OK_u64(4, 3058874); }
  { BYTES(0x96, 0xF2, 0x9A, 0x0A); OK_u64(4, 21412118); }
  { BYTES(0x9A, 0x9F, 0xBC, 0x47); OK_u64(4, 149884826); }
  { BYTES(0xB6, 0xDA, 0xA5, 0xF4, 0x03); OK_u64(5, 1049193782); }
  { BYTES(0xFA, 0xF8, 0x87, 0xAE, 0x1B); OK_u64(5, 7344356474); }
  { BYTES(0xD6, 0xCE, 0xB7, 0xC2, 0xBF, 0x01); OK_u64(6, 51410495318); }
  { BYTES(0xDA, 0xA6, 0x85, 0xD1, 0xBC, 0x0A); OK_u64(6, 359873467226); }
  { BYTES(0xF6, 0x8E, 0xA5, 0xB7, 0xA8, 0x49); OK_u64(6, 2519114270582); }
  { BYTES(0xBA, 0xE8, 0x83, 0x83, 0x9B, 0x81, 0x04); OK_u64(7, 17633799894074); }
  { BYTES(0x96, 0xDB, 0x9A, 0x95, 0xBD, 0x88, 0x1C); OK_u64(7, 123436599258518); }
  { BYTES(0x9A, 0xFE, 0xBA, 0x94, 0xAC, 0xBB, 0xC4, 0x01); OK_u64(8, 864056194809626); }
  { BYTES(0xB6, 0xF3, 0x9C, 0x8F, 0xB5, 0x9F, 0xDF, 0x0A); OK_u64(8, 6048393363667382); }
  
  { BYTES(0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x02); ERR_u64; }
  { BYTES(0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x80, 0x01); ERR_u64; }
  return 1;
}

int test_u64ext() {
  { BYTES(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01); OK_u64(10, 0x8000000000000000uL); }
  { BYTES(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00); OK_u64(10, 0); }

  { BYTES(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x02); ERR_u64; }
  { BYTES(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x04); ERR_u64; }
  { BYTES(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x08); ERR_u64; }
  { BYTES(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x10); ERR_u64; }
  { BYTES(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x20); ERR_u64; }
  { BYTES(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x40); ERR_u64; }

  { BYTES(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80); ERR_u64; }
  { BYTES(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x50); ERR_u64; }
  { BYTES(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x20); ERR_u64; }
  { BYTES(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x30); ERR_u64; }

  { BYTES(0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x80, 0x00); ERR_u64; }
  { BYTES(0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x80, 0x01); ERR_u64; }
  return 1;
}

test_t all_tests[] = {
  {"i32leb", test_i32},
  {"i32leb_ext", test_i32ext},
  {"u32leb", test_u32},
  {"u32leb_ext", test_u32ext},
  {"i64leb", test_i64},
  {"i64leb_ext", test_i64ext},
  {"u64leb", test_u64},
  {"u64leb_ext", test_u64ext},
};

//================================================================================
// Generic test-run code

int run_tests() {
  int count = sizeof(all_tests) / sizeof(test_t);
  int failed = 0;
  printf("##>%d\n", count);
  for (int i = 0; i < count; i++) {
    test_t* t = &all_tests[i];
    printf("##+%s\n", t->name);
    if (t->run() == 0) {
      failed++;
      printf("##-fail\n");
    } else {
      printf("##-ok\n");
    }
  }
  return failed;
}

      
int foo() {
  { BYTES(0x80, 0x01); OK_i32(2, 128); }
  return 1;
}
