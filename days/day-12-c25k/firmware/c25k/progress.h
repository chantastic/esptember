#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

namespace c25k {
// The packed layout is explicit: 10 bytes per entry; no compiler padding in NVS.
struct __attribute__((packed)) LogEntry {
  uint8_t workout;
  uint32_t timestamp;
  uint16_t total_sec;
  uint16_t run_sec;
  uint8_t flags;
};
static_assert(sizeof(LogEntry) == 10, "Stable log format");

struct __attribute__((packed)) Progress {
  uint32_t magic;
  uint8_t schema;
  uint8_t cursor;
  uint8_t sound_on;
  uint8_t vib_on;
  uint8_t program_complete;
  uint8_t reserved;
  uint16_t count;
  uint16_t head;  // Next write position, oldest when full.
  LogEntry entries[256];
  uint32_t checksum;

  void defaults() {
    memset(this, 0, sizeof(*this));
    magic = 0x4332354b; schema = 1; sound_on = 1; vib_on = 1;
    seal();
  }
  uint32_t hash() const {
    const auto* bytes = reinterpret_cast<const uint8_t*>(this);
    uint32_t result = 2166136261u;
    for (size_t i = 0; i < offsetof(Progress, checksum); ++i)
      result = (result ^ bytes[i]) * 16777619u;
    return result;
  }
  void seal() { checksum = hash(); }
  bool valid() const {
    if (magic != 0x4332354b || schema != 1 || cursor > 26 || count > 256 ||
        head > 255 || sound_on > 1 || vib_on > 1 || program_complete > 1 ||
        (count < 256 && head != count) || checksum != hash()) return false;
    for (uint16_t i = 0; i < count; ++i)
      if (entries[i].workout > 26 || entries[i].flags > 1 ||
          entries[i].run_sec > entries[i].total_sec) return false;
    return true;
  }
  const LogEntry* newest(uint16_t offset) const {
    return offset < count ? &entries[(head + 255 - offset) % 256] : nullptr;
  }
  void append(const LogEntry& entry) {
    entries[head] = entry;
    head = (head + 1) % 256;
    if (count < 256) ++count;
    cursor = entry.workout < 26 ? entry.workout + 1 : 26;
    program_complete = entry.workout == 26;
    seal();
  }
  void resetProgress() {
    cursor = 0; count = 0; head = 0; program_complete = 0;
    memset(entries, 0, sizeof(entries)); seal();
  }
};

inline uint16_t logSeconds(uint64_t ms) {
  const uint64_t sec = ms / 1000;
  return sec > 65535 ? 65535 : static_cast<uint16_t>(sec);
}
}  // namespace c25k
