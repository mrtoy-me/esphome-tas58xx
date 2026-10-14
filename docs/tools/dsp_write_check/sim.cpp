// simulated DSP memory: compares the old per biquad writer with dsp_write_bytes_ for every run of biquads
// (one I2C write for each page segment of a run)
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <map>
#include <vector>
#define ESP_LOGE(...) do {} while (0)
#define ESP_LOGW(...) do {} while (0)
#define ESP_LOGD(...) do {} while (0)
[[maybe_unused]] static const char *TAG = "t";
[[maybe_unused]] static const char *ERROR = "e";
static constexpr uint8_t TAS58XX_PAGE_SET = 0x00, TAS58XX_BOOK_SET = 0x7F, TAS58XX_BOOK_ZERO = 0, TAS58XX_PAGE_ZERO = 0;
static constexpr uint8_t BIQUAD_SIZE = 20;
namespace i2c { enum ErrorCode { ERROR_OK = 0, ERROR_UNKNOWN = 7 }; }
struct Tas58xxComponent {
  uint8_t book{0}, page{0};
  std::map<uint32_t, uint8_t> mem;
  size_t max_chunk{0}, txns{0};
  bool bad_align{false};
  uint8_t base_sub{0};
  bool tas58xx_write_byte_(uint8_t reg, uint8_t v) {
    txns++;
    if (reg == TAS58XX_PAGE_SET) { page = v; return true; }
    if (reg == TAS58XX_BOOK_SET && page == 0) { book = v; return true; }
    mem[(book << 16) | (page << 8) | reg] = v; return true;
  }
  bool write_byte(uint8_t reg, uint8_t v) { return tas58xx_write_byte_(reg, v); }
  // one I2C write: register (subaddress) followed by the data
  i2c::ErrorCode write(const uint8_t *buf, size_t len) {
    txns++; max_chunk = std::max<size_t>(max_chunk, len - 1);
    const uint8_t reg = buf[0]; const size_t n = len - 1;
    if (reg + n > 0x80 || reg < 0x08) { printf("write past page: reg %02x n %zu\n", reg, n); return i2c::ERROR_UNKNOWN; }
    if ((reg - 0x08) % 4 || n % 4) bad_align = true;
    for (size_t i = 0; i < n; i++) mem[(book << 16) | (page << 8) | (reg + i)] = buf[1 + i];
    return i2c::ERROR_OK;
  }
  // used by the old writer
  bool tas58xx_write_bytes_(uint8_t reg, const uint8_t *d, uint8_t n) {
    uint8_t buf[32]; buf[0] = reg; for (int i = 0; i < n; i++) buf[1 + i] = d[i];
    max_chunk = 0; return write(buf, n + 1) == i2c::ERROR_OK;
  }
  bool set_book_and_page_(uint8_t b, uint8_t p) {
    return tas58xx_write_byte_(TAS58XX_PAGE_SET, 0) && tas58xx_write_byte_(TAS58XX_BOOK_SET, b) && tas58xx_write_byte_(TAS58XX_PAGE_SET, p);
  }
  bool biquad_write_bytes_(uint8_t book, uint8_t page, uint8_t sub_addr, uint8_t* biquad, uint8_t number_bytes);
  bool dsp_write_bytes_(uint8_t book, uint8_t page, uint8_t sub_addr, const uint8_t *data, size_t length);
};
#include "old_fn.inc"  // biquad_write_bytes_ from the reference commit
#include "new_fn.inc"  // dsp_write_bytes_ from the working tree
struct AddressSequence { uint8_t page, sub_addr; };
AddressSequence addr(AddressSequence first, int ch, int idx) {
  const uint16_t off = (first.sub_addr - 8) + (ch * 15 + idx) * 20;
  return {static_cast<uint8_t>(first.page + off / 0x78), static_cast<uint8_t>(8 + off % 0x78)};
}
int main() {
  const AddressSequence firsts[2] = {{0x24, 0x18}, {0x01, 0x30}};  // TAS5805M, TAS5825M
  int runs = 0, fails = 0; size_t old_tx = 0, new_tx = 0, maxc = 0;
  for (auto first : firsts)
    for (int ch = 0; ch < 2; ch++)
      for (int start = 0; start < 15; start++)
        for (int count = 1; start + count <= 15; count++) {
          uint8_t data[15 * 20];
          for (int i = 0; i < count * 20; i++) data[i] = static_cast<uint8_t>((i * 37 + start * 11 + ch * 101 + count) & 0xFF);
          Tas58xxComponent o, n;
          for (int b = 0; b < count; b++) {
            auto a = addr(first, ch, start + b);
            o.biquad_write_bytes_(0xAA, a.page, a.sub_addr, data + b * 20, 20);
          }
          auto a = addr(first, ch, start);
          bool ok = n.dsp_write_bytes_(0xAA, a.page, a.sub_addr, data, count * 20);
          runs++;
          if (!ok || o.mem != n.mem || n.book != 0 || n.page != 0 || n.bad_align) { fails++; printf("FAIL ch%d start%d count%d\n", ch, start, count); }
          if (count == 15 && start == 0) { old_tx += o.txns; new_tx += n.txns; }
          maxc = std::max(maxc, n.max_chunk);
        }
  printf("runs %d fails %d, largest I2C write %zu data bytes, full 15 band channel transactions old %zu new %zu (2 chips x 2 channels)\n", runs, fails, maxc, old_tx, new_tx);
  // single biquad (runtime gain change), preset run of 3, 8 byte and 16 byte writes
  Tas58xxComponent s; auto a = addr(firsts[0], 0, 0); uint8_t d[20] = {};
  s.dsp_write_bytes_(0xAA, a.page, a.sub_addr, d, 20); printf("one biquad: %zu transactions\n", s.txns);
}
