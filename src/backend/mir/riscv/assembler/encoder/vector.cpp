// Translated from /workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/assembler/encoder/vector.rs
// Self-contained so it can be validated without the yet-unified RISC-V encoder headers.

#include <cctype>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace c4c::backend::riscv::assembler::encoder {

struct Operand {
  struct Reg {
    std::string name;
  };
  struct Imm {
    std::int64_t value;
  };
  struct Symbol {
    std::string name;
  };
  struct Mem {
    std::string base;
    std::int64_t offset;
  };

  using Value = std::variant<Reg, Imm, Symbol, Mem>;
  Value value;

  Operand(Reg v) : value(std::move(v)) {}
  Operand(Imm v) : value(std::move(v)) {}
  Operand(Symbol v) : value(std::move(v)) {}
  Operand(Mem v) : value(std::move(v)) {}
};

struct EncodeResult {
  std::uint32_t word = 0;
};

template <typename T>
using Result = std::variant<T, std::string>;

template <typename T>
static Result<T> ok(T value) {
  return Result<T>(std::in_place_type<T>, std::move(value));
}

template <typename T>
static Result<T> err(std::string message) {
  return Result<T>(std::in_place_type<std::string>, std::move(message));
}

static constexpr std::uint32_t OP_LOAD_FP = 0b0000111;
static constexpr std::uint32_t OP_STORE_FP = 0b0100111;
static constexpr std::uint32_t OP_V = 0b1010111;
static constexpr std::uint32_t OP_V_CRYPTO = 0b1110111;

static std::string lower_copy(std::string_view text) {
  std::string out(text);
  for (char& ch : out) {
    if (ch >= 'A' && ch <= 'Z') {
      ch = static_cast<char>(ch - 'A' + 'a');
    }
  }
  return out;
}

static std::string operand_desc(const Operand& operand) {
  return std::visit(
      [](const auto& value) -> std::string {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, Operand::Reg>) {
          return "reg(" + value.name + ")";
        } else if constexpr (std::is_same_v<T, Operand::Imm>) {
          return "imm(" + std::to_string(value.value) + ")";
        } else if constexpr (std::is_same_v<T, Operand::Symbol>) {
          return "sym(" + value.name + ")";
        } else if constexpr (std::is_same_v<T, Operand::Mem>) {
          return "mem(" + value.base + "," + std::to_string(value.offset) + ")";
        }
        return std::string("operand");
      },
      operand.value);
}

static std::optional<std::uint32_t> reg_num(const std::string& name) {
  const std::string lower = lower_copy(name);
  if (lower == "zero") return 0;
  if (lower == "ra") return 1;
  if (lower == "sp") return 2;
  if (lower == "gp") return 3;
  if (lower == "tp") return 4;
  if (lower == "t0") return 5;
  if (lower == "t1") return 6;
  if (lower == "t2") return 7;
  if (lower == "s0" || lower == "fp") return 8;
  if (lower == "s1") return 9;
  if (lower == "a0") return 10;
  if (lower == "a1") return 11;
  if (lower == "a2") return 12;
  if (lower == "a3") return 13;
  if (lower == "a4") return 14;
  if (lower == "a5") return 15;
  if (lower == "a6") return 16;
  if (lower == "a7") return 17;
  if (lower == "s2") return 18;
  if (lower == "s3") return 19;
  if (lower == "s4") return 20;
  if (lower == "s5") return 21;
  if (lower == "s6") return 22;
  if (lower == "s7") return 23;
  if (lower == "s8") return 24;
  if (lower == "s9") return 25;
  if (lower == "s10") return 26;
  if (lower == "s11") return 27;
  if (lower == "t3") return 28;
  if (lower == "t4") return 29;
  if (lower == "t5") return 30;
  if (lower == "t6") return 31;

  if (!lower.empty() && lower.front() == 'x') {
    std::uint32_t value = 0;
    for (std::size_t i = 1; i < lower.size(); ++i) {
      const char ch = lower[i];
      if (ch < '0' || ch > '9') return std::nullopt;
      value = value * 10 + static_cast<std::uint32_t>(ch - '0');
    }
    if (value <= 31) return value;
  }
  return std::nullopt;
}

static std::optional<std::uint32_t> vreg_num(const std::string& name) {
  const std::string lower = lower_copy(name);
  if (lower.size() < 2 || lower.front() != 'v') return std::nullopt;

  std::uint32_t value = 0;
  for (std::size_t i = 1; i < lower.size(); ++i) {
    const char ch = lower[i];
    if (ch < '0' || ch > '9') return std::nullopt;
    value = value * 10 + static_cast<std::uint32_t>(ch - '0');
  }
  return value <= 31 ? std::optional<std::uint32_t>(value) : std::nullopt;
}

static Result<std::uint32_t> get_reg(const std::vector<Operand>& operands, std::size_t idx) {
  if (idx >= operands.size()) {
    return err<std::uint32_t>("expected register at operand " + std::to_string(idx) + ", got none");
  }
  const Operand& operand = operands[idx];
  if (const auto* reg = std::get_if<Operand::Reg>(&operand.value)) {
    const auto value = reg_num(reg->name);
    if (value.has_value()) return ok<std::uint32_t>(*value);
    return err<std::uint32_t>("invalid integer register: " + reg->name);
  }
  if (const auto* imm = std::get_if<Operand::Imm>(&operand.value)) {
    if (imm->value >= 0 && imm->value <= 31) {
      return ok<std::uint32_t>(static_cast<std::uint32_t>(imm->value));
    }
  }
  return err<std::uint32_t>("expected register at operand " + std::to_string(idx) + ", got " +
                             operand_desc(operand));
}

static Result<std::uint32_t> get_vreg(const std::vector<Operand>& operands, std::size_t idx) {
  if (idx >= operands.size()) {
    return err<std::uint32_t>("expected vector register at operand " + std::to_string(idx) +
                              ", got none");
  }
  const Operand& operand = operands[idx];
  if (const auto* reg = std::get_if<Operand::Reg>(&operand.value)) {
    const auto value = vreg_num(reg->name);
    if (value.has_value()) return ok<std::uint32_t>(*value);
    return err<std::uint32_t>("invalid vector register: " + reg->name);
  }
  return err<std::uint32_t>("expected vector register at operand " + std::to_string(idx) + ", got " +
                             operand_desc(operand));
}

static Result<std::int64_t> get_imm(const std::vector<Operand>& operands, std::size_t idx) {
  if (idx >= operands.size()) {
    return err<std::int64_t>("expected immediate at operand " + std::to_string(idx) + ", got none");
  }
  const Operand& operand = operands[idx];
  if (const auto* imm = std::get_if<Operand::Imm>(&operand.value)) {
    return ok<std::int64_t>(imm->value);
  }
  return err<std::int64_t>("expected immediate at operand " + std::to_string(idx) + ", got " +
                           operand_desc(operand));
}

static Result<std::uint32_t> parse_vtypei(const std::vector<Operand>& operands,
                                          std::size_t start_idx) {
  std::uint32_t sew = 0;
  std::uint32_t lmul = 0;
  std::uint32_t ta = 0;
  std::uint32_t ma = 0;

  for (std::size_t i = start_idx; i < operands.size(); ++i) {
    const auto& operand = operands[i];
    std::string name;
    if (const auto* sym = std::get_if<Operand::Symbol>(&operand.value)) {
      name = lower_copy(sym->name);
    } else if (const auto* reg = std::get_if<Operand::Reg>(&operand.value)) {
      name = lower_copy(reg->name);
    } else if (const auto* imm = std::get_if<Operand::Imm>(&operand.value)) {
      return ok<std::uint32_t>(static_cast<std::uint32_t>(imm->value) & 0x7ffu);
    } else {
      continue;
    }

    if (name == "e8") {
      sew = 0b000;
    } else if (name == "e16") {
      sew = 0b001;
    } else if (name == "e32") {
      sew = 0b010;
    } else if (name == "e64") {
      sew = 0b011;
    } else if (name == "m1") {
      lmul = 0b000;
    } else if (name == "m2") {
      lmul = 0b001;
    } else if (name == "m4") {
      lmul = 0b010;
    } else if (name == "m8") {
      lmul = 0b011;
    } else if (name == "mf2") {
      lmul = 0b111;
    } else if (name == "mf4") {
      lmul = 0b110;
    } else if (name == "mf8") {
      lmul = 0b101;
    } else if (name == "ta") {
      ta = 1;
    } else if (name == "tu") {
      ta = 0;
    } else if (name == "ma") {
      ma = 1;
    } else if (name == "mu") {
      ma = 0;
    } else {
      return err<std::uint32_t>("unknown vtypei field: " + name);
    }
  }

  return ok<std::uint32_t>((ma << 7) | (ta << 6) | (sew << 3) | lmul);
}

static Result<EncodeResult> make_word_result(std::uint32_t word) {
  return ok<EncodeResult>(EncodeResult{word});
}

Result<EncodeResult> encode_vsetvli(const std::vector<Operand>& operands) {
  const auto rd = get_reg(operands, 0);
  if (std::holds_alternative<std::string>(rd)) return err<EncodeResult>(std::get<std::string>(rd));
  const auto rs1 = get_reg(operands, 1);
  if (std::holds_alternative<std::string>(rs1)) return err<EncodeResult>(std::get<std::string>(rs1));
  const auto vtypei = parse_vtypei(operands, 2);
  if (std::holds_alternative<std::string>(vtypei)) return err<EncodeResult>(std::get<std::string>(vtypei));

  const std::uint32_t word = ((std::get<std::uint32_t>(vtypei) & 0x7ffu) << 20) |
                             (std::get<std::uint32_t>(rs1) << 15) | (0b111u << 12) |
                             (std::get<std::uint32_t>(rd) << 7) | OP_V;
  return make_word_result(word);
}

Result<EncodeResult> encode_vsetivli(const std::vector<Operand>& operands) {
  const auto rd = get_reg(operands, 0);
  if (std::holds_alternative<std::string>(rd)) return err<EncodeResult>(std::get<std::string>(rd));
  const auto uimm = get_imm(operands, 1);
  if (std::holds_alternative<std::string>(uimm)) return err<EncodeResult>(std::get<std::string>(uimm));
  const auto vtypei = parse_vtypei(operands, 2);
  if (std::holds_alternative<std::string>(vtypei)) return err<EncodeResult>(std::get<std::string>(vtypei));

  const std::uint32_t word = (0b11u << 30) |
                             ((std::get<std::uint32_t>(vtypei) & 0x3ffu) << 20) |
                             ((static_cast<std::uint32_t>(std::get<std::int64_t>(uimm)) & 0x1fu) << 15) |
                             (0b111u << 12) | (std::get<std::uint32_t>(rd) << 7) | OP_V;
  return make_word_result(word);
}

Result<EncodeResult> encode_vsetvl(const std::vector<Operand>& operands) {
  const auto rd = get_reg(operands, 0);
  if (std::holds_alternative<std::string>(rd)) return err<EncodeResult>(std::get<std::string>(rd));
  const auto rs1 = get_reg(operands, 1);
  if (std::holds_alternative<std::string>(rs1)) return err<EncodeResult>(std::get<std::string>(rs1));
  const auto rs2 = get_reg(operands, 2);
  if (std::holds_alternative<std::string>(rs2)) return err<EncodeResult>(std::get<std::string>(rs2));

  const std::uint32_t word = (0b1000000u << 25) | (std::get<std::uint32_t>(rs2) << 20) |
                             (std::get<std::uint32_t>(rs1) << 15) | (0b111u << 12) |
                             (std::get<std::uint32_t>(rd) << 7) | OP_V;
  return make_word_result(word);
}

Result<EncodeResult> encode_vload(const std::vector<Operand>& operands, std::uint32_t width,
                                  std::uint32_t lumop) {
  const auto vd = get_vreg(operands, 0);
  if (std::holds_alternative<std::string>(vd)) return err<EncodeResult>(std::get<std::string>(vd));

  if (operands.size() < 2) {
    return err<EncodeResult>("expected (rs1) at operand 1, got none");
  }

  std::uint32_t rs1 = 0;
  const Operand& second = operands[1];
  if (const auto* mem = std::get_if<Operand::Mem>(&second.value)) {
    if (mem->offset != 0) {
      return err<EncodeResult>("expected (rs1) at operand 1, got " + operand_desc(second));
    }
    const auto base = reg_num(mem->base);
    if (!base.has_value()) {
      return err<EncodeResult>("invalid base register: " + mem->base);
    }
    rs1 = *base;
  } else if (const auto* reg = std::get_if<Operand::Reg>(&second.value)) {
    const auto base = reg_num(reg->name);
    if (!base.has_value()) {
      return err<EncodeResult>("invalid register: " + reg->name);
    }
    rs1 = *base;
  } else {
    return err<EncodeResult>("expected (rs1) at operand 1, got " + operand_desc(second));
  }

  const std::uint32_t word = (1u << 25) | (lumop << 20) | (rs1 << 15) | (width << 12) |
                             (std::get<std::uint32_t>(vd) << 7) | OP_LOAD_FP;
  return make_word_result(word);
}

Result<EncodeResult> encode_vstore(const std::vector<Operand>& operands, std::uint32_t width,
                                   std::uint32_t sumop) {
  const auto vs3 = get_vreg(operands, 0);
  if (std::holds_alternative<std::string>(vs3)) return err<EncodeResult>(std::get<std::string>(vs3));

  if (operands.size() < 2) {
    return err<EncodeResult>("expected (rs1) at operand 1, got none");
  }

  std::uint32_t rs1 = 0;
  const Operand& second = operands[1];
  if (const auto* mem = std::get_if<Operand::Mem>(&second.value)) {
    if (mem->offset != 0) {
      return err<EncodeResult>("expected (rs1) at operand 1, got " + operand_desc(second));
    }
    const auto base = reg_num(mem->base);
    if (!base.has_value()) {
      return err<EncodeResult>("invalid base register: " + mem->base);
    }
    rs1 = *base;
  } else if (const auto* reg = std::get_if<Operand::Reg>(&second.value)) {
    const auto base = reg_num(reg->name);
    if (!base.has_value()) {
      return err<EncodeResult>("invalid register: " + reg->name);
    }
    rs1 = *base;
  } else {
    return err<EncodeResult>("expected (rs1) at operand 1, got " + operand_desc(second));
  }

  const std::uint32_t word = (1u << 25) | (sumop << 20) | (rs1 << 15) | (width << 12) |
                             (std::get<std::uint32_t>(vs3) << 7) | OP_STORE_FP;
  return make_word_result(word);
}

Result<EncodeResult> encode_v_arith_vv(const std::vector<Operand>& operands, std::uint32_t funct6) {
  const auto vd = get_vreg(operands, 0);
  if (std::holds_alternative<std::string>(vd)) return err<EncodeResult>(std::get<std::string>(vd));
  const auto vs2 = get_vreg(operands, 1);
  if (std::holds_alternative<std::string>(vs2)) return err<EncodeResult>(std::get<std::string>(vs2));
  const auto vs1 = get_vreg(operands, 2);
  if (std::holds_alternative<std::string>(vs1)) return err<EncodeResult>(std::get<std::string>(vs1));

  const std::uint32_t word = (funct6 << 26) | (1u << 25) | (std::get<std::uint32_t>(vs2) << 20) |
                             (std::get<std::uint32_t>(vs1) << 15) | (std::get<std::uint32_t>(vd) << 7) | OP_V;
  return make_word_result(word);
}

Result<EncodeResult> encode_v_arith_vx(const std::vector<Operand>& operands, std::uint32_t funct6) {
  const auto vd = get_vreg(operands, 0);
  if (std::holds_alternative<std::string>(vd)) return err<EncodeResult>(std::get<std::string>(vd));
  const auto vs2 = get_vreg(operands, 1);
  if (std::holds_alternative<std::string>(vs2)) return err<EncodeResult>(std::get<std::string>(vs2));
  const auto rs1 = get_reg(operands, 2);
  if (std::holds_alternative<std::string>(rs1)) return err<EncodeResult>(std::get<std::string>(rs1));

  const std::uint32_t word =
      (funct6 << 26) | (1u << 25) | (std::get<std::uint32_t>(vs2) << 20) |
      (std::get<std::uint32_t>(rs1) << 15) | (0b100u << 12) | (std::get<std::uint32_t>(vd) << 7) | OP_V;
  return make_word_result(word);
}

Result<EncodeResult> encode_v_arith_vi(const std::vector<Operand>& operands, std::uint32_t funct6) {
  const auto vd = get_vreg(operands, 0);
  if (std::holds_alternative<std::string>(vd)) return err<EncodeResult>(std::get<std::string>(vd));
  const auto vs2 = get_vreg(operands, 1);
  if (std::holds_alternative<std::string>(vs2)) return err<EncodeResult>(std::get<std::string>(vs2));
  const auto simm5 = get_imm(operands, 2);
  if (std::holds_alternative<std::string>(simm5)) return err<EncodeResult>(std::get<std::string>(simm5));

  const std::uint32_t word =
      (funct6 << 26) | (1u << 25) | (std::get<std::uint32_t>(vs2) << 20) |
      ((static_cast<std::uint32_t>(std::get<std::int64_t>(simm5)) & 0x1fu) << 15) |
      (0b011u << 12) | (std::get<std::uint32_t>(vd) << 7) | OP_V;
  return make_word_result(word);
}

Result<EncodeResult> encode_vmv_v_v(const std::vector<Operand>& operands) {
  const auto vd = get_vreg(operands, 0);
  if (std::holds_alternative<std::string>(vd)) return err<EncodeResult>(std::get<std::string>(vd));
  const auto vs1 = get_vreg(operands, 1);
  if (std::holds_alternative<std::string>(vs1)) return err<EncodeResult>(std::get<std::string>(vs1));

  const std::uint32_t word =
      (0b010111u << 26) | (1u << 25) | (std::get<std::uint32_t>(vs1) << 15) |
      (std::get<std::uint32_t>(vd) << 7) | OP_V;
  return make_word_result(word);
}

Result<EncodeResult> encode_vmv_v_x(const std::vector<Operand>& operands) {
  const auto vd = get_vreg(operands, 0);
  if (std::holds_alternative<std::string>(vd)) return err<EncodeResult>(std::get<std::string>(vd));
  const auto rs1 = get_reg(operands, 1);
  if (std::holds_alternative<std::string>(rs1)) return err<EncodeResult>(std::get<std::string>(rs1));

  const std::uint32_t word = (0b010111u << 26) | (1u << 25) | (std::get<std::uint32_t>(rs1) << 15) |
                             (0b100u << 12) | (std::get<std::uint32_t>(vd) << 7) | OP_V;
  return make_word_result(word);
}

Result<EncodeResult> encode_vmv_v_i(const std::vector<Operand>& operands) {
  const auto vd = get_vreg(operands, 0);
  if (std::holds_alternative<std::string>(vd)) return err<EncodeResult>(std::get<std::string>(vd));
  const auto simm5 = get_imm(operands, 1);
  if (std::holds_alternative<std::string>(simm5)) return err<EncodeResult>(std::get<std::string>(simm5));

  const std::uint32_t word =
      (0b010111u << 26) | (1u << 25) |
      ((static_cast<std::uint32_t>(std::get<std::int64_t>(simm5)) & 0x1fu) << 15) |
      (0b011u << 12) | (std::get<std::uint32_t>(vd) << 7) | OP_V;
  return make_word_result(word);
}

Result<EncodeResult> encode_vid_v(const std::vector<Operand>& operands) {
  const auto vd = get_vreg(operands, 0);
  if (std::holds_alternative<std::string>(vd)) return err<EncodeResult>(std::get<std::string>(vd));

  const std::uint32_t word = (0b010100u << 26) | (1u << 25) | (0b10001u << 15) |
                             (0b010u << 12) | (std::get<std::uint32_t>(vd) << 7) | OP_V;
  return make_word_result(word);
}

Result<EncodeResult> encode_v_crypto_vi(const std::vector<Operand>& operands, std::uint32_t funct6) {
  const auto vd = get_vreg(operands, 0);
  if (std::holds_alternative<std::string>(vd)) return err<EncodeResult>(std::get<std::string>(vd));
  const auto vs2 = get_vreg(operands, 1);
  if (std::holds_alternative<std::string>(vs2)) return err<EncodeResult>(std::get<std::string>(vs2));
  const auto uimm5 = get_imm(operands, 2);
  if (std::holds_alternative<std::string>(uimm5)) return err<EncodeResult>(std::get<std::string>(uimm5));

  const std::uint32_t word =
      (funct6 << 26) | (1u << 25) | (std::get<std::uint32_t>(vs2) << 20) |
      ((static_cast<std::uint32_t>(std::get<std::int64_t>(uimm5)) & 0x1fu) << 15) |
      (0b010u << 12) | (std::get<std::uint32_t>(vd) << 7) | OP_V_CRYPTO;
  return make_word_result(word);
}

Result<EncodeResult> encode_v_crypto_vv(const std::vector<Operand>& operands, std::uint32_t funct6) {
  const auto vd = get_vreg(operands, 0);
  if (std::holds_alternative<std::string>(vd)) return err<EncodeResult>(std::get<std::string>(vd));
  const auto vs2 = get_vreg(operands, 1);
  if (std::holds_alternative<std::string>(vs2)) return err<EncodeResult>(std::get<std::string>(vs2));
  const auto vs1 = get_vreg(operands, 2);
  if (std::holds_alternative<std::string>(vs1)) return err<EncodeResult>(std::get<std::string>(vs1));

  const std::uint32_t word =
      (funct6 << 26) | (1u << 25) | (std::get<std::uint32_t>(vs2) << 20) |
      (std::get<std::uint32_t>(vs1) << 15) | (0b010u << 12) | (std::get<std::uint32_t>(vd) << 7) |
      OP_V_CRYPTO;
  return make_word_result(word);
}

Result<EncodeResult> encode_v_crypto_vs(const std::vector<Operand>& operands, std::uint32_t funct6) {
  const auto vd = get_vreg(operands, 0);
  if (std::holds_alternative<std::string>(vd)) return err<EncodeResult>(std::get<std::string>(vd));
  const auto vs2 = get_vreg(operands, 1);
  if (std::holds_alternative<std::string>(vs2)) return err<EncodeResult>(std::get<std::string>(vs2));

  const std::uint32_t word =
      (funct6 << 26) | (1u << 25) | (std::get<std::uint32_t>(vs2) << 20) |
      (0b10000u << 15) | (0b010u << 12) | (std::get<std::uint32_t>(vd) << 7) | OP_V_CRYPTO;
  return make_word_result(word);
}

}  // namespace c4c::backend::riscv::assembler::encoder
