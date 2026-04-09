#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace c4c::backend::riscv::codegen {

struct Value {
  std::uint32_t id = 0;
};

struct IrConst {
  struct I128 {
    __int128 value = 0;
  };

  struct LongDouble {
    long double value = 0.0L;
    std::array<std::uint8_t, 16> f128_bytes{};
  };

  std::variant<std::int64_t, long double, I128, LongDouble> payload{};

  std::optional<long double> to_f64() const {
    if (const auto* v = std::get_if<std::int64_t>(&payload)) {
      return static_cast<long double>(*v);
    }
    if (const auto* v = std::get_if<long double>(&payload)) {
      return *v;
    }
    if (const auto* v = std::get_if<I128>(&payload)) {
      return static_cast<long double>(v->value);
    }
    if (const auto* v = std::get_if<LongDouble>(&payload)) {
      return v->value;
    }
    return std::nullopt;
  }
};

using Operand = std::variant<Value, IrConst>;

enum class IrType {
  Void,
  I8,
  I16,
  I32,
  I64,
  U8,
  U16,
  U32,
  U64,
  Ptr,
  F32,
  F64,
  F128,
};

inline bool is_i128_type(IrType ty) {
  return ty == IrType::F128;
}

struct RiscvFloatClass {
  struct OneFloat {
    bool is_double = true;
  };
  struct TwoFloats {
    bool lo_is_double = true;
    bool hi_is_double = true;
  };
  struct IntAndFloat {
    bool float_is_double = true;
    std::size_t int_size = 8;
    std::size_t float_offset = 8;
  };
  struct FloatAndInt {
    bool float_is_double = true;
    std::size_t int_offset = 8;
    std::size_t int_size = 8;
  };

  std::variant<OneFloat, TwoFloats, IntAndFloat, FloatAndInt> payload;
};

struct CallAbiConfig {
  std::size_t max_int_regs = 8;
  std::size_t max_float_regs = 8;
  bool align_i128_pairs = true;
  bool f128_in_fp_regs = false;
  bool f128_in_gp_pairs = true;
  bool variadic_floats_in_gp = true;
  bool large_struct_by_ref = true;
  bool use_sysv_struct_classification = false;
  bool use_riscv_float_struct_classification = true;
  bool allow_struct_split_reg_stack = true;
  bool align_struct_pairs = true;
  bool sret_uses_dedicated_reg = false;
};

struct CallArgClass {
  struct IntReg {
    std::size_t reg_idx = 0;
  };
  struct FloatReg {
    std::size_t reg_idx = 0;
  };
  struct I128RegPair {
    std::size_t base_reg_idx = 0;
  };
  struct F128Reg {
    std::size_t reg_idx = 0;
  };
  struct StructByValReg {
    std::size_t base_reg_idx = 0;
    std::size_t size = 0;
  };
  struct StructSseReg {
    std::size_t lo_fp_idx = 0;
    std::optional<std::size_t> hi_fp_idx;
    std::size_t size = 0;
  };
  struct StructMixedIntSseReg {
    std::size_t int_reg_idx = 0;
    std::size_t fp_reg_idx = 0;
    std::size_t size = 0;
  };
  struct StructMixedSseIntReg {
    std::size_t fp_reg_idx = 0;
    std::size_t int_reg_idx = 0;
    std::size_t size = 0;
  };
  struct StructByValStack {
    std::size_t size = 0;
  };
  struct StructSplitRegStack {
    std::size_t reg_idx = 0;
    std::size_t size = 0;
  };
  struct LargeStructStack {
    std::size_t size = 0;
  };
  struct Stack {};
  struct F128Stack {};
  struct I128Stack {};
  struct ZeroSizeSkip {};

  using Variant = std::variant<IntReg,
                               FloatReg,
                               I128RegPair,
                               F128Reg,
                               StructByValReg,
                               StructSseReg,
                               StructMixedIntSseReg,
                               StructMixedSseIntReg,
                               StructByValStack,
                               StructSplitRegStack,
                               LargeStructStack,
                               Stack,
                               F128Stack,
                               I128Stack,
                               ZeroSizeSkip>;

  Variant value;

  bool is_stack() const {
    return std::visit(
        [](const auto& v) -> bool {
          using T = std::decay_t<decltype(v)>;
          return std::is_same_v<T, Stack> || std::is_same_v<T, F128Stack> ||
                 std::is_same_v<T, I128Stack> || std::is_same_v<T, StructByValStack> ||
                 std::is_same_v<T, LargeStructStack> ||
                 std::is_same_v<T, StructSplitRegStack>;
        },
        value);
  }

  std::size_t stack_bytes() const {
    constexpr std::size_t slot_size = 8;
    constexpr std::size_t align_mask = slot_size - 1;
    return std::visit(
        [align_mask](const auto& v) -> std::size_t {
          using T = std::decay_t<decltype(v)>;
          if constexpr (std::is_same_v<T, F128Stack>) {
            return 16;
          } else if constexpr (std::is_same_v<T, I128Stack>) {
            return 16;
          } else if constexpr (std::is_same_v<T, StructByValStack> ||
                               std::is_same_v<T, LargeStructStack>) {
            return (v.size + align_mask) & ~align_mask;
          } else if constexpr (std::is_same_v<T, StructSplitRegStack>) {
            const auto stack_part = v.size - slot_size;
            return (stack_part + align_mask) & ~align_mask;
          } else if constexpr (std::is_same_v<T, Stack>) {
            return slot_size;
          } else {
            return 0;
          }
        },
        value);
  }
};

constexpr std::array<const char*, 8> RISCV_ARG_REGS = {
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7"};

constexpr std::array<const char*, 12> RISCV_CALLEE_SAVED = {
    "s1", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", nullptr};

inline std::string_view callee_saved_name(std::size_t reg) {
  switch (reg) {
    case 1: return "s1";
    case 2: return "s2";
    case 3: return "s3";
    case 4: return "s4";
    case 5: return "s5";
    case 6: return "s6";
    case 7: return "s7";
    case 8: return "s8";
    case 9: return "s9";
    case 10: return "s10";
    case 11: return "s11";
    default: return {};
  }
}

inline std::optional<std::size_t> riscv_reg_to_callee_saved(std::string_view name) {
  if (name == "s1" || name == "x9") return 1;
  if (name == "s2" || name == "x18") return 2;
  if (name == "s3" || name == "x19") return 3;
  if (name == "s4" || name == "x20") return 4;
  if (name == "s5" || name == "x21") return 5;
  if (name == "s6" || name == "x22") return 6;
  if (name == "s7" || name == "x23") return 7;
  if (name == "s8" || name == "x24") return 8;
  if (name == "s9" || name == "x25") return 9;
  if (name == "s10" || name == "x26") return 10;
  if (name == "s11" || name == "x27") return 11;
  return std::nullopt;
}

inline std::optional<std::size_t> constraint_to_callee_saved_riscv(std::string_view constraint) {
  if (constraint.size() >= 2 && constraint.front() == '{' && constraint.back() == '}') {
    return riscv_reg_to_callee_saved(constraint.substr(1, constraint.size() - 2));
  }
  return riscv_reg_to_callee_saved(constraint);
}

inline std::size_t compute_stack_arg_space(const std::vector<CallArgClass>& arg_classes) {
  std::size_t total = 0;
  for (const auto& cls : arg_classes) {
    if (!cls.is_stack()) {
      continue;
    }
    if (std::holds_alternative<CallArgClass::F128Stack>(cls.value) ||
        std::holds_alternative<CallArgClass::I128Stack>(cls.value)) {
      total = (total + 15) & ~std::size_t(15);
    }
    total += cls.stack_bytes();
  }
  return (total + 15) & ~std::size_t(15);
}

inline CallAbiConfig call_abi_config_impl() {
  return CallAbiConfig{
      .max_int_regs = 8,
      .max_float_regs = 8,
      .align_i128_pairs = true,
      .f128_in_fp_regs = false,
      .f128_in_gp_pairs = true,
      .variadic_floats_in_gp = true,
      .large_struct_by_ref = true,
      .use_sysv_struct_classification = false,
      .use_riscv_float_struct_classification = true,
      .allow_struct_split_reg_stack = true,
      .align_struct_pairs = true,
      .sret_uses_dedicated_reg = false,
  };
}

// The remaining Rust methods depend on the wider RiscvCodegen / CodegenState
// surface that is not yet shared as C++ headers. They are kept here as the
// translated method inventory, but intentionally stop short of inventing a
// cross-file fake backend API.
//
// - emit_call_compute_stack_space_impl
// - emit_call_f128_pre_convert_impl
// - emit_call_stack_args_impl
// - emit_call_reg_args_impl
// - emit_call_instruction_impl
// - emit_call_cleanup_impl
// - emit_call_store_result_impl
//
// The concrete ABI logic above is the part that is practical to translate
// standalone without widening scope into the rest of the backend.

}  // namespace c4c::backend::riscv::codegen
