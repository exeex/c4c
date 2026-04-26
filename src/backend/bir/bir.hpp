#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "../../shared/text_id_table.hpp"

namespace c4c::backend::bir {

struct Module;

struct NameTables {
  NameTables() { reattach(); }

  NameTables(const NameTables& other) : texts(other.texts), block_labels(other.block_labels) {
    reattach();
  }

  NameTables(NameTables&& other) noexcept
      : texts(std::move(other.texts)), block_labels(std::move(other.block_labels)) {
    reattach();
  }

  NameTables& operator=(const NameTables& other) {
    if (this == &other) {
      return *this;
    }
    texts = other.texts;
    block_labels = other.block_labels;
    reattach();
    return *this;
  }

  NameTables& operator=(NameTables&& other) noexcept {
    if (this == &other) {
      return *this;
    }
    texts = std::move(other.texts);
    block_labels = std::move(other.block_labels);
    reattach();
    return *this;
  }

  TextTable texts;
  BlockLabelTable block_labels{&texts};

 private:
  void reattach() { block_labels.attach_text_table(&texts); }
};

enum class TypeKind : unsigned char {
  Void,
  I1,
  I8,
  I16,
  I32,
  I64,
  I128,
  Ptr,
  F32,
  F64,
  F128,
};

enum class AddressSpace : unsigned char {
  Default,
  Fs,
  Gs,
  Tls,
};

enum class CallingConv : unsigned char {
  C,
  SysV,
  Win64,
  Fast,
  Cold,
};

enum class AbiValueClass : unsigned char {
  None,
  Integer,
  Sse,
  X87,
  Memory,
};

struct Value {
  enum class Kind : unsigned char {
    Immediate,
    Named,
  };

  Kind kind = Kind::Immediate;
  TypeKind type = TypeKind::Void;
  std::int64_t immediate = 0;
  std::uint64_t immediate_bits = 0;
  std::string name;

  static Value immediate_i1(bool value);
  static Value immediate_i8(std::int8_t value);
  static Value immediate_i16(std::int16_t value);
  static Value immediate_i32(std::int32_t value);
  static Value immediate_i64(std::int64_t value);
  static Value immediate_f32_bits(std::uint32_t bits);
  static Value immediate_f64_bits(std::uint64_t bits);
  static Value named(TypeKind type, std::string name);
};

inline bool operator==(const Value& lhs, const Value& rhs) {
  return lhs.kind == rhs.kind && lhs.type == rhs.type && lhs.immediate == rhs.immediate &&
         lhs.immediate_bits == rhs.immediate_bits && lhs.name == rhs.name;
}

inline bool operator!=(const Value& lhs, const Value& rhs) {
  return !(lhs == rhs);
}

struct PhiIncoming {
  std::string label;
  Value value;
  BlockLabelId label_id = kInvalidBlockLabel;
};

struct PhiObservation {
  Value result;
  std::vector<PhiIncoming> incomings;
};

struct CallArgAbiInfo {
  TypeKind type = TypeKind::Void;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
  AbiValueClass primary_class = AbiValueClass::None;
  AbiValueClass secondary_class = AbiValueClass::None;
  bool passed_in_register = false;
  bool passed_on_stack = false;
  bool byval_copy = false;
  bool sret_pointer = false;
};

struct CallResultAbiInfo {
  TypeKind type = TypeKind::Void;
  AbiValueClass primary_class = AbiValueClass::None;
  AbiValueClass secondary_class = AbiValueClass::None;
  bool returned_in_memory = false;
};

struct Param {
  TypeKind type = TypeKind::Void;
  std::string name;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
  std::optional<CallArgAbiInfo> abi;
  bool is_varargs = false;
  bool is_sret = false;
  bool is_byval = false;
};

enum class LocalSlotStorageKind : unsigned char {
  None,
  LoweringScratch,
};

struct LocalSlot {
  std::string name;
  TypeKind type = TypeKind::Void;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
  LocalSlotStorageKind storage_kind = LocalSlotStorageKind::None;
  bool is_address_taken = false;
  bool is_byval_copy = false;
  std::optional<PhiObservation> phi_observation;
};

struct Global {
  std::string name;
  TypeKind type = TypeKind::Void;
  bool is_extern = false;
  bool is_thread_local = false;
  bool is_constant = false;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
  std::optional<Value> initializer;
  std::optional<std::string> initializer_symbol_name;
  std::vector<Value> initializer_elements;
};

struct StringConstant {
  std::string name;
  std::string bytes;
  std::size_t align_bytes = 1;
};

struct StructuredTypeFieldSpelling {
  std::string type_name;
};

struct StructuredTypeDeclSpelling {
  std::string name;
  std::vector<StructuredTypeFieldSpelling> fields;
  bool is_packed = false;
  bool is_opaque = false;
};

struct StructuredTypeSpellingContext {
  std::vector<StructuredTypeDeclSpelling> declarations;

  [[nodiscard]] const StructuredTypeDeclSpelling* find_struct_decl(
      std::string_view name) const;
};

struct MemoryAddress {
  enum class BaseKind : unsigned char {
    None,
    LocalSlot,
    GlobalSymbol,
    PointerValue,
    Label,
    StringConstant,
  };

  BaseKind base_kind = BaseKind::None;
  std::string base_name;
  Value base_value;
  std::int64_t byte_offset = 0;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
  AddressSpace address_space = AddressSpace::Default;
  bool is_volatile = false;
  BlockLabelId base_label_id = kInvalidBlockLabel;
};

enum class BinaryOpcode : unsigned char {
  Add,
  Sub,
  Mul,
  And,
  Or,
  Xor,
  Shl,
  LShr,
  AShr,
  SDiv,
  UDiv,
  SRem,
  URem,
  Eq,
  Ne,
  Slt,
  Sle,
  Sgt,
  Sge,
  Ult,
  Ule,
  Ugt,
  Uge,
};

struct BinaryInst {
  BinaryOpcode opcode = BinaryOpcode::Add;
  Value result;
  TypeKind operand_type = TypeKind::Void;
  Value lhs;
  Value rhs;
};

struct SelectInst {
  BinaryOpcode predicate = BinaryOpcode::Eq;
  Value result;
  TypeKind compare_type = TypeKind::Void;
  Value lhs;
  Value rhs;
  Value true_value;
  Value false_value;
};

enum class CastOpcode : unsigned char {
  SExt,
  ZExt,
  Trunc,
  FPTrunc,
  FPExt,
  FPToSI,
  FPToUI,
  SIToFP,
  UIToFP,
  PtrToInt,
  IntToPtr,
  Bitcast,
};

struct CastInst {
  CastOpcode opcode = CastOpcode::SExt;
  Value result;
  Value operand;
};

struct PhiInst {
  Value result;
  std::vector<PhiIncoming> incomings;
};

struct InlineAsmMetadata {
  std::string asm_text;
  std::string constraints;
  std::string args_text;
  bool side_effects = false;
};

struct CallInst {
  std::optional<Value> result;
  std::string callee;
  std::optional<Value> callee_value;
  std::vector<Value> args;
  std::vector<TypeKind> arg_types;
  std::vector<CallArgAbiInfo> arg_abi;
  std::optional<std::string> structured_return_type_name;
  std::string return_type_name;
  TypeKind return_type = TypeKind::Void;
  std::optional<CallResultAbiInfo> result_abi;
  CallingConv calling_convention = CallingConv::C;
  bool is_indirect = false;
  bool is_variadic = false;
  bool is_noreturn = false;
  std::optional<InlineAsmMetadata> inline_asm;
  std::optional<std::string> sret_storage_name;
};

struct LoadLocalInst {
  Value result;
  std::string slot_name;
  std::size_t byte_offset = 0;
  std::size_t align_bytes = 0;
  std::optional<MemoryAddress> address;
};

struct LoadGlobalInst {
  Value result;
  std::string global_name;
  std::size_t byte_offset = 0;
  std::size_t align_bytes = 0;
  std::optional<MemoryAddress> address;
};

struct StoreGlobalInst {
  std::string global_name;
  Value value;
  std::size_t byte_offset = 0;
  std::size_t align_bytes = 0;
  std::optional<MemoryAddress> address;
};

struct StoreLocalInst {
  std::string slot_name;
  Value value;
  std::size_t byte_offset = 0;
  std::size_t align_bytes = 0;
  std::optional<MemoryAddress> address;
};

using Inst = std::variant<BinaryInst,
                          SelectInst,
                          CastInst,
                          PhiInst,
                          CallInst,
                          LoadLocalInst,
                          LoadGlobalInst,
                          StoreGlobalInst,
                          StoreLocalInst>;

struct ReturnTerminator {
  std::optional<Value> value;
};

struct BranchTerminator {
  std::string target_label;
  BlockLabelId target_label_id = kInvalidBlockLabel;
};

struct CondBranchTerminator {
  Value condition;
  std::string true_label;
  std::string false_label;
  BlockLabelId true_label_id = kInvalidBlockLabel;
  BlockLabelId false_label_id = kInvalidBlockLabel;
};

enum class TerminatorKind : unsigned char {
  Return,
  Branch,
  CondBranch,
};

struct Terminator {
  TerminatorKind kind = TerminatorKind::Return;
  std::optional<Value> value;
  Value condition;
  std::string target_label;
  BlockLabelId target_label_id = kInvalidBlockLabel;
  std::string true_label;
  BlockLabelId true_label_id = kInvalidBlockLabel;
  std::string false_label;
  BlockLabelId false_label_id = kInvalidBlockLabel;

  Terminator() = default;
  Terminator(const ReturnTerminator& ret) : kind(TerminatorKind::Return), value(ret.value) {}
  Terminator(const BranchTerminator& br)
      : kind(TerminatorKind::Branch),
        target_label(br.target_label),
        target_label_id(br.target_label_id) {}
  Terminator(const CondBranchTerminator& br)
      : kind(TerminatorKind::CondBranch),
        condition(br.condition),
        true_label(br.true_label),
        true_label_id(br.true_label_id),
        false_label(br.false_label),
        false_label_id(br.false_label_id) {}
};

struct Block {
  std::string label;
  std::vector<Inst> insts;
  Terminator terminator;
  BlockLabelId label_id = kInvalidBlockLabel;
};

struct Function {
  std::string name;
  TypeKind return_type = TypeKind::Void;
  std::size_t return_size_bytes = 0;
  std::size_t return_align_bytes = 0;
  std::optional<CallResultAbiInfo> return_abi;
  CallingConv calling_convention = CallingConv::C;
  bool is_variadic = false;
  std::vector<Param> params;
  std::vector<LocalSlot> local_slots;
  std::vector<Block> blocks;
  bool is_declaration = false;
};

struct Module {
  std::string target_triple;
  std::string data_layout;
  StructuredTypeSpellingContext structured_types;
  std::vector<Global> globals;
  std::vector<StringConstant> string_constants;
  std::vector<Function> functions;
  NameTables names;
};

inline bool is_compare_opcode(BinaryOpcode opcode) {
  switch (opcode) {
    case BinaryOpcode::Eq:
    case BinaryOpcode::Ne:
    case BinaryOpcode::Slt:
    case BinaryOpcode::Sle:
    case BinaryOpcode::Sgt:
    case BinaryOpcode::Sge:
    case BinaryOpcode::Ult:
    case BinaryOpcode::Ule:
    case BinaryOpcode::Ugt:
    case BinaryOpcode::Uge:
      return true;
    default:
      return false;
  }
}

inline TypeKind binary_operand_type(const BinaryInst& inst) {
  return inst.operand_type == TypeKind::Void ? inst.result.type : inst.operand_type;
}

inline TypeKind select_compare_type(const SelectInst& inst) {
  return inst.compare_type == TypeKind::Void ? inst.result.type : inst.compare_type;
}

inline std::optional<std::int64_t> parse_i32_return_immediate(const Function& function) {
  if (function.is_declaration || function.return_type != TypeKind::I32 ||
      function.blocks.size() != 1) {
    return std::nullopt;
  }

  const auto& block = function.blocks.front();
  if (block.label != "entry" || !block.insts.empty() ||
      block.terminator.kind != TerminatorKind::Return ||
      !block.terminator.value.has_value() ||
      block.terminator.value->kind != Value::Kind::Immediate ||
      block.terminator.value->type != TypeKind::I32) {
    return std::nullopt;
  }

  return block.terminator.value->immediate;
}

std::string render_type(TypeKind type);
std::string render_binary_opcode(BinaryOpcode opcode);
std::string render_cast_opcode(CastOpcode opcode);
std::string print(const Module& module);
bool validate(const Module& module, std::string* error);

}  // namespace c4c::backend::bir
