// prints the biquads of one filter (type frequency gain Q) as big endian 5.27 hex, for compare.py
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include "tas58xx_helpers.h"
using namespace esphome::tas58xx_helpers;
static uint32_t bs(int32_t v){return __builtin_bswap32((uint32_t)v);}
int main(int argc, char **argv) {
  // args: type freq gain q   (q parsed as float, as the component stores it)
  FilterType t = (FilterType)atoi(argv[1]);
  uint16_t f = atoi(argv[2]); int8_t g = atoi(argv[3]); float q = strtof(argv[4], nullptr);
  BiquadCoefficients b = make_biquad(t, 96000, f, g, q);
  printf("0x%08X 0x%08X 0x%08X 0x%08X 0x%08X\n", bs(b.b0), bs(b.b1), bs(b.b2), bs(b.a1), bs(b.a2));
}
