// dumps every biquad the helpers can produce for a wide parameter space, to compare versions
#include <cstdio>
#include "tas58xx_helpers.h"
using namespace esphome::tas58xx_helpers;
int main() {
  const uint16_t freqs[] = {20,32,50,60,70,80,90,100,110,120,125,130,140,150,200,220,240,260,280,300,315,500,800,1250,2000,3150,5000,8000,16000,
                            1,10,25,40,63,160,250,400,630,1000,1600,2500,4000,6300,10000,20000,24000,30000,47999};
  const float qs[] = {0.5f,0.6f,0.7f,0.8f,0.9f,1.0f,1.5f,2.0f,2.144908f,0.1f,5.0f,10.0f,0.7071f};
  const uint32_t rates[] = {96000, 48000, 44100};
  for (uint32_t sr : rates)
  for (int type = FILTER_TYPE_ALL_PASS; type <= FILTER_TYPE_HIGH_PASS; type++)
    for (uint16_t f : freqs)
      for (int g = -24; g <= 24; g++)
        for (float q : qs) {
          BiquadCoefficients b = make_biquad(static_cast<FilterType>(type), sr, f, static_cast<int8_t>(g), q);
          printf("%u %d %u %d %.6f %08x %08x %08x %08x %08x\n", sr, type, f, g, q, (uint32_t)b.b0,(uint32_t)b.b1,(uint32_t)b.b2,(uint32_t)b.a1,(uint32_t)b.a2);
        }
  for (int g = -48; g <= 48; g++) printf("vol %d %08x\n", g, (uint32_t)gain_to_fixed_9_23(static_cast<int8_t>(g)));
  BiquadCoefficients a = ALL_PASS;
  printf("ap %08x %08x %08x %08x %08x\n", (uint32_t)a.b0,(uint32_t)a.b1,(uint32_t)a.b2,(uint32_t)a.a1,(uint32_t)a.a2);
}
