#include <algorithm>
#include <cstdint>
#include <optional>
#include <unordered_set>
#include <utility>
#include <vector>

namespace c4c::backend::riscv::assembler {

namespace {

inline bool is_creg(std::uint32_t reg) {
  return reg >= 8 && reg <= 15;
}

inline std::uint32_t creg_num(std::uint32_t reg) {
  return reg - 8;
}

template <typename T>
inline bool contains_inclusive(T value, T lo, T hi) {
  return value >= lo && value <= hi;
}

}  // namespace

std::optional<std::uint16_t> try_compress_rv64(std::uint32_t word) {
  const auto opcode = word & 0x7f;
  const auto rd = (word >> 7) & 0x1f;
  const auto funct3 = (word >> 12) & 0x7;
  const auto rs1 = (word >> 15) & 0x1f;
  const auto rs2 = (word >> 20) & 0x1f;
  const auto funct7 = (word >> 25) & 0x7f;

  switch (opcode) {
    case 0b0110111: {
      if (rd == 0 || rd == 2) return std::nullopt;
      auto imm20 = static_cast<std::int32_t>(word >> 12);
      imm20 = (imm20 << 12) >> 12;
      const auto nzimm = imm20;
      if (nzimm == 0) return std::nullopt;
      if (!contains_inclusive(nzimm, -32, 31)) return std::nullopt;
      const auto imm = static_cast<std::uint32_t>(nzimm);
      const auto bit17 = (imm >> 5) & 1;
      const auto bits16_12 = imm & 0x1f;
      return static_cast<std::uint16_t>(0b011'0'00000'00000'01 | (bit17 << 12) |
                                        (rd << 7) | (bits16_12 << 2));
    }

    case 0b0010011:
      switch (funct3) {
        case 0b000: {
          const auto imm = static_cast<std::int32_t>(word) >> 20;
          if (rd == 0 && rs1 == 0 && imm == 0) {
            return static_cast<std::uint16_t>(0b000'0'00000'00000'01);
          }
          if (rd == rs1 && rd != 0 && imm != 0 && contains_inclusive(imm, -32, 31)) {
            const auto nzimm = static_cast<std::uint32_t>(imm);
            const auto bit5 = (nzimm >> 5) & 1;
            const auto bits4_0 = nzimm & 0x1f;
            return static_cast<std::uint16_t>(0b000'0'00000'00000'01 | (bit5 << 12) |
                                              (rd << 7) | (bits4_0 << 2));
          }
          if (rd == 2 && rs1 == 2 && imm != 0 && (imm % 16) == 0 &&
              contains_inclusive(imm, -512, 496)) {
            const auto uimm = static_cast<std::uint32_t>(imm);
            const auto bit9 = (uimm >> 9) & 1;
            const auto bit4 = (uimm >> 4) & 1;
            const auto bit6 = (uimm >> 6) & 1;
            const auto bits8_7 = (uimm >> 7) & 0x3;
            const auto bit5 = (uimm >> 5) & 1;
            return static_cast<std::uint16_t>(0b011'0'00010'00000'01 | (bit9 << 12) |
                                              (bit4 << 6) | (bit6 << 5) |
                                              (bits8_7 << 3) | (bit5 << 2));
          }
          if (rs1 == 2 && rd != 0 && rd != 2 && is_creg(rd) && imm > 0 &&
              (imm % 4) == 0 && imm <= 1020) {
            const auto uimm = static_cast<std::uint32_t>(imm);
            const auto rd_prime = creg_num(rd);
            const auto bits5_4 = (uimm >> 4) & 0x3;
            const auto bits9_6 = (uimm >> 6) & 0xf;
            const auto bit2 = (uimm >> 2) & 1;
            const auto bit3 = (uimm >> 3) & 1;
            return static_cast<std::uint16_t>((bits5_4 << 11) | (bits9_6 << 7) |
                                              (bit2 << 6) | (bit3 << 5) |
                                              (rd_prime << 2));
          }
          if (rs1 == 0 && rd != 0) {
            if (!contains_inclusive(imm, -32, 31)) return std::nullopt;
            const auto imm_u = static_cast<std::uint32_t>(imm);
            const auto bit5 = (imm_u >> 5) & 1;
            const auto bits4_0 = imm_u & 0x1f;
            return static_cast<std::uint16_t>(0b010'0'00000'00000'01 | (bit5 << 12) |
                                              (rd << 7) | (bits4_0 << 2));
          }
          return std::nullopt;
        }

        case 0b001: {
          const auto shamt = (word >> 20) & 0x3f;
          if (rd == rs1 && rd != 0 && shamt != 0) {
            const auto bit5 = (shamt >> 5) & 1;
            const auto bits4_0 = shamt & 0x1f;
            return static_cast<std::uint16_t>(0b000'0'00000'00000'10 | (bit5 << 12) |
                                              (rd << 7) | (bits4_0 << 2));
          }
          return std::nullopt;
        }

        case 0b101: {
          const auto shamt = (word >> 20) & 0x3f;
          const auto is_srai = (funct7 & 0x20) != 0;
          if (rd == rs1 && is_creg(rd) && shamt != 0) {
            const auto rd_prime = creg_num(rd);
            const auto bit5 = (shamt >> 5) & 1;
            const auto bits4_0 = shamt & 0x1f;
            if (is_srai) {
              return static_cast<std::uint16_t>(0b100'0'01'000'00000'01 | (bit5 << 12) |
                                                (rd_prime << 7) | (bits4_0 << 2));
            }
            return static_cast<std::uint16_t>(0b100'0'00'000'00000'01 | (bit5 << 12) |
                                              (rd_prime << 7) | (bits4_0 << 2));
          }
          return std::nullopt;
        }

        case 0b111: {
          const auto imm = static_cast<std::int32_t>(word) >> 20;
          if (rd == rs1 && is_creg(rd)) {
            if (!contains_inclusive(imm, -32, 31)) return std::nullopt;
            const auto rd_prime = creg_num(rd);
            const auto imm_u = static_cast<std::uint32_t>(imm);
            const auto bit5 = (imm_u >> 5) & 1;
            const auto bits4_0 = imm_u & 0x1f;
            return static_cast<std::uint16_t>(0b100'0'10'000'00000'01 | (bit5 << 12) |
                                              (rd_prime << 7) | (bits4_0 << 2));
          }
          return std::nullopt;
        }

        default:
          break;
      }
      break;

    case 0b0110011:
      switch (funct7) {
        case 0b0000000:
          switch (funct3) {
            case 0b000:
              if (rd == rs1 && rd != 0 && rs2 != 0) {
                return static_cast<std::uint16_t>(0b100'1'00000'00000'10 | (rd << 7) |
                                                  (rs2 << 2));
              }
              if (rs1 == 0 && rd != 0 && rs2 != 0) {
                return static_cast<std::uint16_t>(0b100'0'00000'00000'10 | (rd << 7) |
                                                  (rs2 << 2));
              }
              break;
            case 0b100:
            case 0b110:
            case 0b111:
              if (rd == rs1 && is_creg(rd) && is_creg(rs2)) {
                const auto rd_prime = creg_num(rd);
                const auto rs2_prime = creg_num(rs2);
                if (funct3 == 0b100) {
                  return static_cast<std::uint16_t>(0b100'0'11'000'01'000'01 |
                                                    (rd_prime << 7) |
                                                    (rs2_prime << 2));
                }
                if (funct3 == 0b110) {
                  return static_cast<std::uint16_t>(0b100'0'11'000'10'000'01 |
                                                    (rd_prime << 7) |
                                                    (rs2_prime << 2));
                }
                return static_cast<std::uint16_t>(0b100'0'11'000'11'000'01 |
                                                  (rd_prime << 7) |
                                                  (rs2_prime << 2));
              }
              break;
            default:
              break;
          }
          break;

        case 0b0100000:
          if (funct3 == 0b000 && rd == rs1 && is_creg(rd) && is_creg(rs2)) {
            const auto rd_prime = creg_num(rd);
            const auto rs2_prime = creg_num(rs2);
            return static_cast<std::uint16_t>(0b100'0'11'000'00'000'01 | (rd_prime << 7) |
                                              (rs2_prime << 2));
          }
          break;

        default:
          break;
      }
      break;

    case 0b0111011:
      if (funct3 == 0b000 && rd == rs1 && is_creg(rd) && is_creg(rs2)) {
        const auto rd_prime = creg_num(rd);
        const auto rs2_prime = creg_num(rs2);
        if (funct7 == 0b0000000) {
          return static_cast<std::uint16_t>(0b100'1'11'000'01'000'01 | (rd_prime << 7) |
                                            (rs2_prime << 2));
        }
        if (funct7 == 0b0100000) {
          return static_cast<std::uint16_t>(0b100'1'11'000'00'000'01 | (rd_prime << 7) |
                                            (rs2_prime << 2));
        }
      }
      break;

    case 0b0000011:
      if (funct3 == 0b011) {
        const auto imm = static_cast<std::int32_t>(word) >> 20;
        if (rs1 == 2 && rd != 0) {
          if (contains_inclusive(imm, 0, 504) && (imm % 8) == 0) {
            const auto uoff = static_cast<std::uint32_t>(imm);
            const auto bit5 = (uoff >> 5) & 1;
            const auto bits4_3 = (uoff >> 3) & 0x3;
            const auto bits8_6 = (uoff >> 6) & 0x7;
            return static_cast<std::uint16_t>(0b011'0'00000'00000'10 | (bit5 << 12) |
                                              (rd << 7) | (bits4_3 << 5) |
                                              (bits8_6 << 2));
          }
          return std::nullopt;
        }
        if (is_creg(rs1) && is_creg(rd)) {
          if (contains_inclusive(imm, 0, 248) && (imm % 8) == 0) {
            const auto uoff = static_cast<std::uint32_t>(imm);
            const auto rs1_prime = creg_num(rs1);
            const auto rd_prime = creg_num(rd);
            const auto bits5_3 = (uoff >> 3) & 0x7;
            const auto bits7_6 = (uoff >> 6) & 0x3;
            return static_cast<std::uint16_t>(0b011'000'000'00'000'00 |
                                              (bits5_3 << 10) |
                                              (rs1_prime << 7) |
                                              (bits7_6 << 5) |
                                              (rd_prime << 2));
          }
          return std::nullopt;
        }
      } else if (funct3 == 0b010) {
        const auto imm = static_cast<std::int32_t>(word) >> 20;
        if (rs1 == 2 && rd != 0) {
          if (contains_inclusive(imm, 0, 252) && (imm % 4) == 0) {
            const auto uoff = static_cast<std::uint32_t>(imm);
            const auto bit5 = (uoff >> 5) & 1;
            const auto bits4_2 = (uoff >> 2) & 0x7;
            const auto bits7_6 = (uoff >> 6) & 0x3;
            return static_cast<std::uint16_t>(0b010'0'00000'00000'10 | (bit5 << 12) |
                                              (rd << 7) | (bits4_2 << 4) |
                                              (bits7_6 << 2));
          }
          return std::nullopt;
        }
        if (is_creg(rs1) && is_creg(rd)) {
          if (contains_inclusive(imm, 0, 124) && (imm % 4) == 0) {
            const auto uoff = static_cast<std::uint32_t>(imm);
            const auto rs1_prime = creg_num(rs1);
            const auto rd_prime = creg_num(rd);
            const auto bits5_3 = (uoff >> 3) & 0x7;
            const auto bit2 = (uoff >> 2) & 1;
            const auto bit6 = (uoff >> 6) & 1;
            return static_cast<std::uint16_t>(0b010'000'000'00'000'00 |
                                              (bits5_3 << 10) |
                                              (rs1_prime << 7) |
                                              (bit2 << 6) |
                                              (bit6 << 5) |
                                              (rd_prime << 2));
          }
          return std::nullopt;
        }
      }
      break;

    case 0b0100011:
      if (funct3 == 0b011 || funct3 == 0b010) {
        const auto imm11_5 = static_cast<std::int32_t>(word >> 25);
        const auto imm4_0 = static_cast<std::int32_t>((word >> 7) & 0x1f);
        auto imm = (imm11_5 << 5) | imm4_0;
        imm = (imm << 20) >> 20;

        if (funct3 == 0b011) {
          if (rs1 == 2) {
            if (contains_inclusive(imm, 0, 504) && (imm % 8) == 0) {
              const auto uoff = static_cast<std::uint32_t>(imm);
              const auto bits5_3 = (uoff >> 3) & 0x7;
              const auto bits8_6 = (uoff >> 6) & 0x7;
              return static_cast<std::uint16_t>(0b111'000000'00000'10 |
                                                (bits5_3 << 10) |
                                                (bits8_6 << 7) |
                                                (rs2 << 2));
            }
            return std::nullopt;
          }
          if (is_creg(rs1) && is_creg(rs2)) {
            if (contains_inclusive(imm, 0, 248) && (imm % 8) == 0) {
              const auto uoff = static_cast<std::uint32_t>(imm);
              const auto rs1_prime = creg_num(rs1);
              const auto rs2_prime = creg_num(rs2);
              const auto bits5_3 = (uoff >> 3) & 0x7;
              const auto bits7_6 = (uoff >> 6) & 0x3;
              return static_cast<std::uint16_t>(0b111'000'000'00'000'00 |
                                                (bits5_3 << 10) |
                                                (rs1_prime << 7) |
                                                (bits7_6 << 5) |
                                                (rs2_prime << 2));
            }
            return std::nullopt;
          }
        } else {
          if (rs1 == 2) {
            if (contains_inclusive(imm, 0, 252) && (imm % 4) == 0) {
              const auto uoff = static_cast<std::uint32_t>(imm);
              const auto bits5_2 = (uoff >> 2) & 0xf;
              const auto bits7_6 = (uoff >> 6) & 0x3;
              return static_cast<std::uint16_t>(0b110'000000'00000'10 |
                                                (bits5_2 << 9) |
                                                (bits7_6 << 7) |
                                                (rs2 << 2));
            }
            return std::nullopt;
          }
          if (is_creg(rs1) && is_creg(rs2)) {
            if (contains_inclusive(imm, 0, 124) && (imm % 4) == 0) {
              const auto uoff = static_cast<std::uint32_t>(imm);
              const auto rs1_prime = creg_num(rs1);
              const auto rs2_prime = creg_num(rs2);
              const auto bits5_3 = (uoff >> 3) & 0x7;
              const auto bit2 = (uoff >> 2) & 1;
              const auto bit6 = (uoff >> 6) & 1;
              return static_cast<std::uint16_t>(0b110'000'000'00'000'00 |
                                                (bits5_3 << 10) |
                                                (rs1_prime << 7) |
                                                (bit2 << 6) |
                                                (bit6 << 5) |
                                                (rs2_prime << 2));
            }
            return std::nullopt;
          }
        }
      }
      break;

    case 0b0000111:
      if (funct3 == 0b011) {
        const auto imm = static_cast<std::int32_t>(word) >> 20;
        if (rs1 == 2) {
          if (contains_inclusive(imm, 0, 504) && (imm % 8) == 0) {
            const auto uoff = static_cast<std::uint32_t>(imm);
            const auto bit5 = (uoff >> 5) & 1;
            const auto bits4_3 = (uoff >> 3) & 0x3;
            const auto bits8_6 = (uoff >> 6) & 0x7;
            return static_cast<std::uint16_t>(0b001'0'00000'00000'10 | (bit5 << 12) |
                                              (rd << 7) | (bits4_3 << 5) |
                                              (bits8_6 << 2));
          }
          return std::nullopt;
        }
        if (is_creg(rs1) && is_creg(rd)) {
          if (contains_inclusive(imm, 0, 248) && (imm % 8) == 0) {
            const auto uoff = static_cast<std::uint32_t>(imm);
            const auto rs1_prime = creg_num(rs1);
            const auto rd_prime = creg_num(rd);
            const auto bits5_3 = (uoff >> 3) & 0x7;
            const auto bits7_6 = (uoff >> 6) & 0x3;
            return static_cast<std::uint16_t>(0b001'000'000'00'000'00 |
                                              (bits5_3 << 10) |
                                              (rs1_prime << 7) |
                                              (bits7_6 << 5) |
                                              (rd_prime << 2));
          }
          return std::nullopt;
        }
      }
      break;

    case 0b0100111:
      if (funct3 == 0b011) {
        const auto imm11_5 = static_cast<std::int32_t>(word >> 25);
        const auto imm4_0 = static_cast<std::int32_t>((word >> 7) & 0x1f);
        auto imm = (imm11_5 << 5) | imm4_0;
        imm = (imm << 20) >> 20;
        if (rs1 == 2) {
          if (contains_inclusive(imm, 0, 504) && (imm % 8) == 0) {
            const auto uoff = static_cast<std::uint32_t>(imm);
            const auto bits5_3 = (uoff >> 3) & 0x7;
            const auto bits8_6 = (uoff >> 6) & 0x7;
            return static_cast<std::uint16_t>(0b101'000000'00000'10 |
                                              (bits5_3 << 10) |
                                              (bits8_6 << 7) |
                                              (rs2 << 2));
          }
          return std::nullopt;
        }
        if (is_creg(rs1) && is_creg(rs2)) {
          if (contains_inclusive(imm, 0, 248) && (imm % 8) == 0) {
            const auto uoff = static_cast<std::uint32_t>(imm);
            const auto rs1_prime = creg_num(rs1);
            const auto rs2_prime = creg_num(rs2);
            const auto bits5_3 = (uoff >> 3) & 0x7;
            const auto bits7_6 = (uoff >> 6) & 0x3;
            return static_cast<std::uint16_t>(0b101'000'000'00'000'00 |
                                              (bits5_3 << 10) |
                                              (rs1_prime << 7) |
                                              (bits7_6 << 5) |
                                              (rs2_prime << 2));
          }
          return std::nullopt;
        }
      }
      break;

    case 0b1100111: {
      const auto imm = static_cast<std::int32_t>(word) >> 20;
      if (imm == 0 && rs2 == 0) {
        if (rd == 0 && rs1 != 0) {
          return static_cast<std::uint16_t>(0b100'0'00000'00000'10 | (rs1 << 7));
        }
        if (rd == 1 && rs1 != 0) {
          return static_cast<std::uint16_t>(0b100'1'00000'00000'10 | (rs1 << 7));
        }
      }
      return std::nullopt;
    }

    case 0b1101111:
      return std::nullopt;

    case 0b1100011:
      return std::nullopt;

    case 0b1110011:
      if (word == 0x00100073) {
        return static_cast<std::uint16_t>(0b100'1'00000'00000'10);
      }
      return std::nullopt;

    default:
      return std::nullopt;
  }

  return std::nullopt;
}

std::pair<std::vector<std::uint8_t>, std::vector<std::pair<std::uint64_t, std::uint64_t>>>
compress_section(const std::vector<std::uint8_t>& data,
                 const std::unordered_set<std::uint64_t>& reloc_offsets) {
  std::vector<std::uint8_t> new_data;
  new_data.reserve(data.size());
  std::vector<std::pair<std::uint64_t, std::uint64_t>> offset_map;

  std::size_t pos = 0;
  while (pos < data.size()) {
    const auto old_offset = static_cast<std::uint64_t>(pos);
    const auto new_offset = static_cast<std::uint64_t>(new_data.size());
    offset_map.emplace_back(old_offset, new_offset);

    if (pos + 2 <= data.size()) {
      const auto low_byte = data[pos];
      if ((low_byte & 0x03) != 0x03) {
        new_data.insert(new_data.end(), data.begin() + static_cast<std::ptrdiff_t>(pos),
                        data.begin() + static_cast<std::ptrdiff_t>(pos + 2));
        pos += 2;
        continue;
      }
    }

    if (pos + 4 > data.size()) {
      new_data.insert(new_data.end(), data.begin() + static_cast<std::ptrdiff_t>(pos),
                      data.end());
      pos = data.size();
      break;
    }

    if (reloc_offsets.contains(old_offset)) {
      new_data.insert(new_data.end(), data.begin() + static_cast<std::ptrdiff_t>(pos),
                      data.begin() + static_cast<std::ptrdiff_t>(pos + 4));
      pos += 4;
      continue;
    }

    const auto word = static_cast<std::uint32_t>(data[pos]) |
                      (static_cast<std::uint32_t>(data[pos + 1]) << 8) |
                      (static_cast<std::uint32_t>(data[pos + 2]) << 16) |
                      (static_cast<std::uint32_t>(data[pos + 3]) << 24);

    if (const auto halfword = try_compress_rv64(word); halfword.has_value()) {
      const auto hw = halfword.value();
      new_data.push_back(static_cast<std::uint8_t>(hw & 0xff));
      new_data.push_back(static_cast<std::uint8_t>((hw >> 8) & 0xff));
    } else {
      new_data.insert(new_data.end(), data.begin() + static_cast<std::ptrdiff_t>(pos),
                      data.begin() + static_cast<std::ptrdiff_t>(pos + 4));
    }

    pos += 4;
  }

  if (pos < data.size()) {
    const auto old_offset = static_cast<std::uint64_t>(pos);
    const auto new_offset = static_cast<std::uint64_t>(new_data.size());
    offset_map.emplace_back(old_offset, new_offset);
    new_data.insert(new_data.end(), data.begin() + static_cast<std::ptrdiff_t>(pos),
                    data.end());
  }

  return {std::move(new_data), std::move(offset_map)};
}

std::uint64_t remap_offset(
    std::uint64_t offset,
    const std::vector<std::pair<std::uint64_t, std::uint64_t>>& offset_map) {
  const auto it = std::lower_bound(
      offset_map.begin(), offset_map.end(), offset,
      [](const auto& entry, std::uint64_t value) { return entry.first < value; });

  if (it != offset_map.end() && it->first == offset) {
    return it->second;
  }

  if (it == offset_map.begin()) return offset;

  const auto& [prev_old, prev_new] = *(it - 1);
  const auto delta = offset - prev_old;
  return prev_new + delta;
}

}  // namespace c4c::backend::riscv::assembler
