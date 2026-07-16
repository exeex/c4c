#pragma once

// Public LIR package index.
//
// External codegen and backend users should include this header for the LIR
// data model plus printer/verifier entry points. `operands.hpp` and
// `types.hpp` remain top-level model subheaders because this index uses their
// typed wrappers throughout the instruction definitions.

// ── LIR: Low-level IR between HIR and LLVM text emission ────────────────────
//
// This header defines the minimal LIR data model for the HIR→LIR→Printer
// refactor (Milestone D).  The goal is to make the lowering boundary explicit
// and structured, rather than having HirEmitter build LLVM IR strings directly.
//
// Design principles:
//   1. LIR is target-neutral (no LLVM textual syntax here).
//   2. LIR values use explicit IDs; the printer maps them to %tN / @g names.
//   3. Every block has an explicit terminator.
//   4. C type info (TypeSpec) is preserved for later backends.
//   5. This first skeleton is intentionally narrow — only enough to represent
//      what hir_emitter.cpp already lowers.

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <array>
#include <deque>
#include <initializer_list>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include "identity.hpp"
#include "operands.hpp"
#include "types.hpp"
#include "call_args.hpp"
#include "ast.hpp"  // TypeSpec, TypeBase
#include "../../frontend/hir/hir_ir.hpp"
#include "../../shared/struct_name_table.hpp"
#include "../../shared/text_id_table.hpp"
#include "../../target_profile.hpp"

namespace c4c::codegen::lir {

// ── Value / Block / Slot IDs ─────────────────────────────────────────────────

struct LirBlockId {
  uint32_t value = 0;
  static constexpr uint32_t kInvalid = std::numeric_limits<uint32_t>::max();
  [[nodiscard]] constexpr bool valid() const { return value != kInvalid; }
  [[nodiscard]] static constexpr LirBlockId invalid() { return LirBlockId{kInvalid}; }
};

[[nodiscard]] constexpr bool operator==(LirBlockId lhs, LirBlockId rhs) {
  return lhs.value == rhs.value;
}

// Identifies one ordered successor occurrence in a predecessor terminator's
// typed CFG structure. This is deliberately independent of PHI input order
// and of rendered block labels: conditional true/false and repeated switch
// targets are distinct occurrences even when they name the same block.
struct LirSuccessorOccurrenceId {
  uint32_t value = 0;
  static constexpr uint32_t kInvalid = std::numeric_limits<uint32_t>::max();
  [[nodiscard]] constexpr bool valid() const { return value != kInvalid; }
  [[nodiscard]] static constexpr LirSuccessorOccurrenceId invalid() {
    return LirSuccessorOccurrenceId{kInvalid};
  }
  [[nodiscard]] static constexpr LirSuccessorOccurrenceId direct_branch() {
    return LirSuccessorOccurrenceId{0};
  }
  [[nodiscard]] static constexpr LirSuccessorOccurrenceId conditional_true() {
    return LirSuccessorOccurrenceId{0};
  }
  [[nodiscard]] static constexpr LirSuccessorOccurrenceId conditional_false() {
    return LirSuccessorOccurrenceId{1};
  }
  [[nodiscard]] static constexpr LirSuccessorOccurrenceId switch_default() {
    return LirSuccessorOccurrenceId{0};
  }
  [[nodiscard]] static constexpr LirSuccessorOccurrenceId switch_case(uint32_t case_index) {
    return LirSuccessorOccurrenceId{case_index + 1};
  }
};

[[nodiscard]] constexpr bool operator==(LirSuccessorOccurrenceId lhs,
                                        LirSuccessorOccurrenceId rhs) {
  return lhs.value == rhs.value;
}

struct LirStackSlotId {
  uint32_t value = 0;
  static constexpr uint32_t kInvalid = std::numeric_limits<uint32_t>::max();
  [[nodiscard]] constexpr bool valid() const { return value != kInvalid; }
  [[nodiscard]] static constexpr LirStackSlotId invalid() { return LirStackSlotId{kInvalid}; }
};

struct LirGlobalId {
  uint32_t value = 0;
  static constexpr uint32_t kInvalid = std::numeric_limits<uint32_t>::max();
  [[nodiscard]] constexpr bool valid() const { return value != kInvalid; }
  [[nodiscard]] static constexpr LirGlobalId invalid() { return LirGlobalId{kInvalid}; }
};

// Current-function authority for one selected local object.  The pointer
// definition, object identity, owner, pointee type, and liveness are semantic
// facts; an instruction's spelling remains a rendering mirror only.
struct LirCurrentFunctionLocalObjectPointer {
  LirValueId pointer_definition = LirValueId::invalid();
  LirObjectId object = LirObjectId::invalid();
  LinkNameId owner = kInvalidLinkName;
  LirTypeRef pointer_type = LirTypeRef(LirBuiltinType::Pointer);
  LirTypeRef pointee_type;
  bool live = false;
  // Present only for a selected static local-array authority; it records the
  // native element type reached by one indexed GEP without reparsing text.
  std::optional<LirTypeRef> indexed_element_type;
};

// ── Instructions (non-terminator) ────────────────────────────────────────────
//
// Each instruction produces zero or one result value (identified by LirValueId).
// These are intentionally minimal stubs for Stage 0; concrete fields will be
// populated in Stage 1 when string sinks are replaced.

struct LirConstInt {
  LirValueId result{};
  TypeSpec type{};
  long long value = 0;
};

struct LirConstFloat {
  LirValueId result{};
  TypeSpec type{};
  double value = 0.0;
};

// LLVM label addresses are constants, not instructions.  Keep them owned by
// the enclosing function because both their target block and value identity are
// function-local.
struct LirDirectLabelAddressConstant {
  LinkNameId owner = kInvalidLinkName;
  LirBlockId target = LirBlockId::invalid();
  LirTypeRef type = LirTypeRef(LirBuiltinType::Pointer);
  LirValueId value = LirValueId::invalid();
};

struct LirLoad {
  LirValueId result{};
  TypeSpec type{};
  LirValueId ptr{};
};

struct LirStore {
  LirValueId ptr{};
  LirValueId val{};
  TypeSpec type{};
};

struct LirBinary {
  LirValueId result{};
  TypeSpec type{};
  int op = 0;  // will map to BinaryOp or LLVM-level opcode
  LirValueId lhs{};
  LirValueId rhs{};
};

struct LirCast {
  LirValueId result{};
  TypeSpec from_type{};
  TypeSpec to_type{};
  LirValueId operand{};
};

struct LirCmp {
  LirValueId result{};
  int predicate = 0;
  LirValueId lhs{};
  LirValueId rhs{};
};

struct LirCall {
  LirValueId result{};
  TypeSpec return_type{};
  std::string callee_name;
  LirValueId callee_ptr{};  // for indirect calls
  std::vector<LirValueId> args;
};

struct LirGep {
  LirValueId result{};
  TypeSpec base_type{};
  LirValueId base_ptr{};
  std::vector<LirValueId> indices;
};

struct LirSelect {
  LirValueId result{};
  TypeSpec type{};
  LirValueId cond{};
  LirValueId true_val{};
  LirValueId false_val{};
};

struct LirIntrinsic {
  LirValueId result{};
  std::string name;
  std::vector<LirValueId> args;
};

// ── Typed intrinsic operations (Stage 3) ────────────────────────────────────
// Typed intrinsic call operations.
// Operands use string SSA names (matching current emitter convention).

// Typed semantic authority for the one selected fixed-aggregate byval
// materialization memcpy.  The display operands on LirMemcpyOp remain solely
// compatibility spelling; this descriptor is deliberately optional so every
// unselected memcpy producer remains outside the selected authority family.
struct LirSelectedMemcpyAuthority {
  LirValueId destination{};
  LirValueId source{};
  LirTypeRef size_type = LirTypeRef::integer(64);
  LirIntegerImmediate size{};
  LirObjectId destination_object = LirObjectId::invalid();
  LirObjectId source_object = LirObjectId::invalid();
  LinkNameId destination_object_owner = kInvalidLinkName;
  LinkNameId source_object_owner = kInvalidLinkName;
  bool destination_live_at_site = false;
  bool source_live_at_site = false;
};

// Operation-local native authority for the opt-in memory/VA family.  The
// operand remains a display mirror; this record binds it to the current
// function's checked local pointer/object/owner/type/liveness tuple.
struct LirMemoryVaPointerAuthority {
  LirCurrentFunctionLocalObjectPointer local_pointer;
};

struct LirMemoryVaIntegerAuthority {
  LirTypeRef type;
  LirIntegerImmediate value;
};

// Semantic receipt for the one AMD64 SysV aggregate va_arg overflow memcpy.
// The overflow area is derived storage, not a fabricated local object.
enum class LirAmd64SysVOverflowStorage : std::uint8_t {
  Amd64SysVOverflowArgArea,
};

struct LirAmd64SysVOverflowAggregateCarrier {
  LirCurrentFunctionLocalObjectPointer va_list_object;
  LirValueId overflow_field_address = LirValueId::invalid();
  LirValueId overflow_pointer_load = LirValueId::invalid();
  LirAmd64SysVOverflowStorage storage =
      LirAmd64SysVOverflowStorage::Amd64SysVOverflowArgArea;
  LirCurrentFunctionLocalObjectPointer destination;
  LirValueId final_load = LirValueId::invalid();
  LirTypeRef payload_type;
  LirTypeRef payload_size_type = LirTypeRef::integer(64);
  LirIntegerImmediate payload_size;
};

struct LirMemcpyOp {
  LirOperand dst;         // ptr operand
  LirOperand src;         // ptr operand
  LirOperand size;        // i64 operand
  bool is_volatile = false;
  std::optional<LirSelectedMemcpyAuthority> selected_authority;
  bool requires_native_memory_va_authority = false;
  std::optional<LirMemoryVaPointerAuthority> dst_authority;
  std::optional<LirMemoryVaPointerAuthority> src_authority;
  std::optional<LirMemoryVaIntegerAuthority> size_authority;
  std::optional<LirAmd64SysVOverflowAggregateCarrier>
      amd64_sysv_overflow_aggregate_carrier;
};

struct LirVaStartOp {
  LirOperand ap_ptr;      // ptr operand
  bool requires_native_memory_va_authority = false;
  std::optional<LirMemoryVaPointerAuthority> ap_authority;
};

struct LirVaEndOp {
  LirOperand ap_ptr;      // ptr operand
  bool requires_native_memory_va_authority = false;
  std::optional<LirMemoryVaPointerAuthority> ap_authority;
};

struct LirVaCopyOp {
  LirOperand dst_ptr;     // ptr operand
  LirOperand src_ptr;     // ptr operand
  bool requires_native_memory_va_authority = false;
  std::optional<LirMemoryVaPointerAuthority> dst_authority;
  std::optional<LirMemoryVaPointerAuthority> src_authority;
};

struct LirStackSaveOp {
  LirOperand result;      // SSA name for saved stack pointer
  std::optional<LirCurrentFunctionLocalObjectPointer> local_object_authority;
  // Opt-in receipt for the one selected VLA saved-stack-pointer producer.
  // Compatibility stack saves remain unselected and fail closed downstream.
  bool requires_native_stack_save_authority = false;
};

struct LirStackRestoreOp {
  LirOperand saved_ptr;   // SSA name of saved stack pointer
  std::optional<LirCurrentFunctionLocalObjectPointer> local_object_authority;
  // Opt-in receipt for the selected VLA saved-stack-pointer consumer.
  // Compatibility stack restores remain unselected.
  bool requires_native_stack_restore_authority = false;
  struct LirStackRestoreLifetimeTransition {
    enum class Kind { RestoreSavedVlaStackCheckpoint } kind;
    LirValueId saved_pointer_definition = LirValueId::invalid();
  };
  std::optional<LirStackRestoreLifetimeTransition> lifetime_transition;
};

struct LirAbsOp {
  LirOperand result;      // SSA name for result
  LirOperand arg;         // SSA name for input
  LirTypeRef int_type;    // e.g. "i32" or "i64"
};

struct LirInlineAsm {
  LirValueId result{};
  std::string asm_string;
  std::string constraints;
  std::vector<LirValueId> operands;
};


struct LirIndirectBrOp {
  LirOperand addr;                    // checked display mirror of addr_value
  std::optional<LirValueId> addr_value;  // current-function pointer authority
  std::vector<std::string> targets;   // display labels (e.g. "ulbl_foo")
  // Semantic CFG authority in display order; labels remain mirrors only.
  std::vector<LirBlockId> successors;
};

struct LirExtractValueOp {
  LirOperand result;      // SSA name for result
  LirTypeRef agg_type;    // aggregate type string (e.g. "{ double, double }")
  LirOperand agg;         // SSA name of aggregate value
  int index = 0;          // field index
  // Opt-in result/use identity for the closed-798 aggregate operand handoff.
  bool requires_native_result_authority = false;
  // Opt-in selected-field type fact for native aggregate extraction.  This is
  // independent of the rendered aggregate spelling and exists only on the
  // direct-complex row that publishes structured extraction semantics.
  std::optional<LirTypeRef> result_element_type;
};

struct LirInsertValueOp {
  LirOperand result;      // SSA name for result
  LirTypeRef agg_type;    // aggregate type string
  LirOperand agg;         // SSA name of aggregate value (or "undef")
  LirTypeRef elem_type;   // element type string
  LirOperand elem;        // SSA name of element to insert
  int index = 0;          // field index
  // Opt-in native aggregate producer authority for the selected terminal row.
  bool requires_native_result_authority = false;
  std::optional<LirTypeRef> aggregate_result_type;
};

struct LirLoadOp {
  LirOperand result;      // SSA name for result (e.g. "%t5")
  LirTypeRef type_str;    // LLVM type string (e.g. "i32", "ptr")
  LirOperand ptr;         // SSA name of pointer operand
  // Opt-in standalone native result ownership; compatibility loads remain false.
  bool requires_native_result_authority = false;
  std::optional<LirCurrentFunctionLocalObjectPointer> local_object_authority;
  std::optional<LirTypeRef> aggregate_result_type;
};

struct LirStoreOp {
  LirTypeRef type_str;    // LLVM type string (e.g. "i32", "ptr")
  LirOperand val;         // SSA name of value (or "zeroinitializer")
  LirOperand ptr;         // SSA name of pointer operand
  std::optional<LirCurrentFunctionLocalObjectPointer> local_object_authority;
  // Opt-in receipt for the one selected native local-scalar declaration store.
  bool requires_native_store_authority = false;
};

struct LirMemsetOp {
  LirOperand dst;         // destination pointer
  LirOperand byte_val;    // i8-compatible byte value
  LirOperand size;        // i64 byte count
  bool is_volatile = false;
  bool requires_native_memory_va_authority = false;
  std::optional<LirMemoryVaPointerAuthority> dst_authority;
  std::optional<LirMemoryVaIntegerAuthority> byte_authority;
  std::optional<LirMemoryVaIntegerAuthority> size_authority;
};

// Cast opcode for LirCastOp
enum class LirCastKind : uint8_t {
  Trunc,
  ZExt,
  SExt,
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

struct LirCastOp {
  LirOperand result;      // SSA name for result
  LirCastKind kind{};     // cast opcode
  LirTypeRef from_type;   // LLVM type string of source
  LirOperand operand;     // SSA name of source operand
  LirTypeRef to_type;     // LLVM type string of destination
  // Opt-in ownership contract for a standalone native result producer.
  // Compatibility casts remain false until their producer publishes native
  // result authority.
  bool requires_native_result_authority = false;
};

class LirGepIndex {
 public:
  LirGepIndex() = default;
  LirGepIndex(const char* presentation)
      : LirGepIndex(std::string(presentation)) {}
  LirGepIndex(std::string presentation)
      : presentation_(std::move(presentation)) {}
  LirGepIndex(const LirOperand& presentation)
      : presentation_(presentation.str()) {}

  [[nodiscard]] static LirGepIndex raw(std::string presentation) {
    return LirGepIndex(std::move(presentation));
  }

  [[nodiscard]] static LirGepIndex typed(LirTypeRef type,
                                         LirOperand value) {
    return LirGepIndex(std::move(type), std::move(value));
  }

  [[nodiscard]] bool is_authoritative() const { return authoritative_; }
  [[nodiscard]] const LirTypeRef& type_ref() const { return type_ref_; }
  [[nodiscard]] const LirOperand& value() const { return value_; }
  [[nodiscard]] const std::string& presentation() const {
    return presentation_;
  }
  [[nodiscard]] std::string str() const {
    if (authoritative_) return type_ref_.str() + " " + value_.str();
    return presentation_;
  }

 private:
  LirGepIndex(LirTypeRef type, LirOperand value)
      : type_ref_(std::move(type)),
        value_(std::move(value)),
        authoritative_(true) {}

  LirTypeRef type_ref_;
  LirOperand value_;
  std::string presentation_;
  bool authoritative_ = false;
};

struct LirGepOp {
  LirOperand result;          // SSA name for result
  LirTypeRef element_type;    // LLVM type string (e.g. "i8", "[5 x i8]", "%struct.foo")
  LirOperand ptr;             // SSA name of pointer operand
  bool inbounds = false;      // getelementptr inbounds
  std::vector<LirGepIndex> indices;
  // Opt-in standalone native result ownership; compatibility GEPs remain false.
  bool requires_native_result_authority = false;
  std::optional<LirCurrentFunctionLocalObjectPointer> local_object_authority;
  // Opt-in receipt for the selected direct static-local array GEP with a
  // native integer-immediate index. Other local GEPs remain fail-closed.
  bool requires_native_local_gep_authority = false;
};

struct LirFunctionSignatureRef {
  uint32_t value = std::numeric_limits<uint32_t>::max();
  [[nodiscard]] constexpr bool valid() const {
    return value != std::numeric_limits<uint32_t>::max();
  }
  [[nodiscard]] static constexpr LirFunctionSignatureRef invalid() {
    return LirFunctionSignatureRef{};
  }
};

struct LirCallSignature {
  std::optional<LirTypeRef> return_type_ref;
  LirExtAttr return_ext_attr = LirExtAttr::None;
  std::vector<std::string> fixed_param_types;
  std::vector<LirTypeRef> fixed_param_type_refs;
  bool is_variadic = false;
  bool has_unspecified_params = false;
  bool has_void_param_list = false;
};

// The only body-parameter ABI classes admitted to native body-use authority.
// Each additional class needs its own producer and verifier contract.
enum class LirNativeBodyParameterAbi : uint8_t {
  Invalid,
  DirectPointer,
  DirectScalar,
};

enum class LirFixedDirectCallArgumentParameterRole : uint8_t {
  Invalid,
  FixedDirectCallArgument0,
  FixedDirectCallArgument1,
};

// Native authority for exactly a direct plain scalar current-function
// parameter passed unchanged as a selected fixed direct-call argument. The operand
// spelling is a checked display mirror; selection facts live here.
struct LirFixedDirectCallArgumentParameterAuthority {
  LirValueId value = LirValueId::invalid();
  uint32_t parameter_index = 0;
  LirTypeRef type;
  LinkNameId owner = kInvalidLinkName;
  LirNativeBodyParameterAbi abi = LirNativeBodyParameterAbi::Invalid;
  LirFixedDirectCallArgumentParameterRole role =
      LirFixedDirectCallArgumentParameterRole::Invalid;
};

struct LirCallArg {
  std::string type;
  LirOperand operand;
  LirTypeRef type_ref;
  std::size_t aarch64_hfa_lane_count = 0;
  std::size_t aarch64_hfa_lane_index = 0;
  std::size_t aarch64_stack_align_bytes = 0;
  LirExtAttr ext_attr = LirExtAttr::None;
  std::optional<LirFixedDirectCallArgumentParameterAuthority>
      fixed_direct_call_argument_parameter_authority;
};

enum class LirIntrinsicKind : unsigned char {
  Cttz,
  Ctlz,
  Ctpop,
};

enum class LirZeroCountBehavior : unsigned char {
  Defined,
  Undefined,
};

enum class LirDirectZeroArgScalarFloatingCallRole : uint8_t {
  Invalid,
  ResultIntoFloatingBinaryLhs,
};

struct LirDirectZeroArgScalarFloatingCallAuthority {
  LirValueId result = LirValueId::invalid();
  LinkNameId owner = kInvalidLinkName;
  LinkNameId callee = kInvalidLinkName;
  LirTypeRef return_type;
  LirDirectZeroArgScalarFloatingCallRole role =
      LirDirectZeroArgScalarFloatingCallRole::Invalid;
};

enum class LirDirectOneDoubleArgScalarFloatingCallRole : uint8_t {
  Invalid,
  DirectCallResult,
};

// Native authority for exactly one direct nonvariadic double(double) call-result
// row.  The argument and callee spelling remain checked mirrors.
struct LirDirectOneDoubleArgScalarFloatingCallAuthority {
  LirValueId result = LirValueId::invalid();
  LinkNameId owner = kInvalidLinkName;
  LinkNameId callee = kInvalidLinkName;
  LirTypeRef return_type;
  LirTypeRef argument_type;
  LirDirectOneDoubleArgScalarFloatingCallRole role =
      LirDirectOneDoubleArgScalarFloatingCallRole::Invalid;
};

// Typed call instruction.
// Covers both direct calls, indirect calls, and intrinsic calls.
struct LirCallOp {
  LirOperand result;               // SSA name for result (empty for void calls)
  LirTypeRef return_type;          // LLVM return type string (e.g. "i32", "void", "{ i32, i1 }")
  LirOperand callee;               // callee value (e.g. "@foo", "%ptr", "@llvm.fabs.f64")
  LinkNameId direct_callee_link_name_id = kInvalidLinkName;
  std::string callee_type_suffix;  // optional fn ptr type suffix (empty for direct calls)
  std::string args_str;            // pre-formatted argument string (e.g. "i32 %t1, i32 %t2")
  std::vector<LirTypeRef> arg_type_refs;  // Mirrors argument type fragments when available
  std::optional<LirCallSignature> callee_signature;  // Structured callee signature when available.
  std::vector<LirCallArg> structured_args;  // Generated argument facts; empty for raw compatibility.
  LirExtAttr return_ext_attr = LirExtAttr::None;
  std::optional<LirIntrinsicKind> intrinsic_kind;
  std::optional<LirZeroCountBehavior> zero_count_behavior;
  // Opt-in standalone native result ownership; compatibility calls remain false.
  bool requires_native_result_authority = false;
  LirFunctionSignatureRef callee_signature_ref =
      LirFunctionSignatureRef::invalid();
  std::optional<LirDirectZeroArgScalarFloatingCallAuthority>
      direct_zero_arg_scalar_floating_call_authority;
  std::optional<LirDirectOneDoubleArgScalarFloatingCallAuthority>
      direct_one_double_arg_scalar_floating_call_authority;
};

enum class LirScalarBinaryParameterRole : uint8_t {
  Invalid,
  Lhs,
  Rhs,
};

enum class LirTruthinessComparisonLhsParameterRole : uint8_t {
  Invalid,
  TruthinessComparisonLhs,
};

enum class LirPointerTruthinessParameterRole : uint8_t {
  Invalid,
  PointerTruthiness,
};

// Native authority for exactly a direct plain scalar current-function
// parameter used as the LHS of this binary operation.  The operand spelling
// is a checked display mirror only; all selection facts live here.
struct LirScalarBinaryLhsParameterAuthority {
  LirValueId value = LirValueId::invalid();
  uint32_t parameter_index = 0;
  LirTypeRef type;
  LinkNameId owner = kInvalidLinkName;
  LirNativeBodyParameterAbi abi = LirNativeBodyParameterAbi::Invalid;
  LirScalarBinaryParameterRole role = LirScalarBinaryParameterRole::Invalid;
};

// Native authority for exactly a direct plain scalar current-function
// parameter used as the RHS of this binary operation.  The operand spelling
// is a checked display mirror only; all selection facts live here.
struct LirScalarBinaryRhsParameterAuthority {
  LirValueId value = LirValueId::invalid();
  uint32_t parameter_index = 0;
  LirTypeRef type;
  LinkNameId owner = kInvalidLinkName;
  LirNativeBodyParameterAbi abi = LirNativeBodyParameterAbi::Invalid;
  LirScalarBinaryParameterRole role = LirScalarBinaryParameterRole::Invalid;
};

// Native authority for exactly a direct plain scalar current-function integer
// parameter used as the LHS of its truthiness comparison. The comparison
// relation itself remains checked on LirCmpOp; display spellings are mirrors.
struct LirTruthinessComparisonLhsParameterAuthority {
  LirValueId value = LirValueId::invalid();
  uint32_t parameter_index = 0;
  LirTypeRef type;
  LinkNameId owner = kInvalidLinkName;
  LirNativeBodyParameterAbi abi = LirNativeBodyParameterAbi::Invalid;
  LirTruthinessComparisonLhsParameterRole role =
      LirTruthinessComparisonLhsParameterRole::Invalid;
};

// Native authority for exactly a direct pointer current-function parameter
// used as a truthiness condition through the existing PtrToInt + icmp-ne-zero
// lowering. The comparison LHS is the cast result; this tuple preserves the
// original parameter value and required consumer shape.
struct LirPointerTruthinessParameterAuthority {
  LirValueId value = LirValueId::invalid();
  uint32_t parameter_index = 0;
  LirTypeRef type;
  LinkNameId owner = kInvalidLinkName;
  LirNativeBodyParameterAbi abi = LirNativeBodyParameterAbi::Invalid;
  LirPointerTruthinessParameterRole role =
      LirPointerTruthinessParameterRole::Invalid;
};

// Typed binary arithmetic/bitwise/unary operation.
// Covers: add, sub, mul, sdiv, udiv, srem, urem, fadd, fsub, fmul, fdiv, frem,
//         and, or, xor, shl, lshr, ashr, fneg.
struct LirBinOp {
  LirOperand result;          // SSA name for result
  LirBinaryOpcodeRef opcode;  // LLVM opcode string (e.g. "add", "fadd", "xor", "fneg")
  LirTypeRef type_str;        // LLVM type string (e.g. "i32", "double", "<4 x i32>")
  LirOperand lhs;             // SSA name or literal for left operand
  LirOperand rhs;             // SSA name or literal for right operand (empty for unary fneg)
  std::optional<LirScalarBinaryLhsParameterAuthority> scalar_lhs_parameter_authority;
  std::optional<LirScalarBinaryRhsParameterAuthority> scalar_rhs_parameter_authority;
  std::optional<LirCompactScalarType> compact_scalar_type =
      LirCompactScalarType::from_type_ref(type_str);
};

// Typed comparison operation (icmp/fcmp).
struct LirCmpOp {
  LirOperand result;              // SSA name for result
  bool is_float = false;          // true = fcmp, false = icmp
  LirCmpPredicateRef predicate;   // predicate string (e.g. "eq", "ne", "slt", "oeq", "uno")
  LirTypeRef type_str;            // LLVM type string of operands
  LirOperand lhs;                 // SSA name or literal for left operand
  LirOperand rhs;                 // SSA name or literal for right operand
  std::optional<LirTruthinessComparisonLhsParameterAuthority>
      truthiness_lhs_parameter_authority;
  std::optional<LirPointerTruthinessParameterAuthority>
      pointer_truthiness_parameter_authority;
};

// PHI incoming authority. `value` selects the native value, `predecessor`
// selects the current-function CFG predecessor, and `successor_occurrence`
// selects one exact successor occurrence in that predecessor's typed
// terminator. `label` is only the predecessor display mirror.
struct LirPhiIncoming {
  LirOperand value;
  std::string label;
  LirBlockId predecessor = LirBlockId::invalid();
  std::optional<LirSuccessorOccurrenceId> successor_occurrence;
};

// Typed PHI node.
struct LirPhiOp {
  LirOperand result;      // SSA name for result
  LirTypeRef type_str;    // LLVM type string
  std::vector<LirPhiIncoming> incoming;
  std::optional<LirPhiBoundaryValueType> boundary_value_type =
      LirPhiBoundaryValueType::from_type_ref(type_str);
};

// Typed select instruction.
struct LirSelectOp {
  LirOperand result;      // SSA name for result
  LirTypeRef type_str;    // LLVM type string for true/false operands
  LirOperand cond;        // SSA name of i1 condition
  LirOperand true_val;    // SSA name or literal for true branch
  LirOperand false_val;   // SSA name or literal for false branch
};

// Opt-in native authority for the three existing vector seams.  The printed
// operands/types remain mirrors; this carrier never derives facts from them.
struct LirNativeVectorShape {
  uint32_t lane_count = 0;
  LirTypeRef element_type;
};

struct LirVectorRef {
  uint32_t value = std::numeric_limits<uint32_t>::max();
  [[nodiscard]] constexpr bool valid() const {
    return value != std::numeric_limits<uint32_t>::max();
  }
  [[nodiscard]] static constexpr LirVectorRef invalid() {
    return LirVectorRef{};
  }
};

struct LirVectorStoreEntry {
  uint32_t lane_count = 0;
  LirTypeRef element_type;
};

struct LirNativeVectorIndex {
  LirOperand value;
  LirTypeRef type;
};

struct LirShuffleMaskLane {
  enum class Kind : uint8_t { Inactive, Selected } kind = Kind::Inactive;
  uint32_t selected_lane = 0;
};

struct LirNativeVectorAuthority {
  LinkNameId owner = kInvalidLinkName;
  LirValueId result = LirValueId::invalid();
  std::optional<LirValueId> first_vector_use;
  std::optional<LirValueId> second_vector_use;
  std::optional<LirValueId> element_use;
  LirNativeVectorShape result_shape;
  std::optional<LirNativeVectorShape> first_vector_shape;
  std::optional<LirNativeVectorShape> second_vector_shape;
  std::optional<LirNativeVectorIndex> index;
  std::vector<LirShuffleMaskLane> mask_lanes;
  std::optional<LirVectorRef> vector_ref;
};

// Typed vector insert/extract/shuffle ops.
struct LirInsertElementOp {
  LirOperand result;      // SSA name for result
  LirTypeRef vec_type;    // vector type string (e.g. "<4 x i32>")
  LirOperand vec;         // SSA name of vector (or "poison")
  LirTypeRef elem_type;   // element type string
  LirOperand elem;        // SSA name of element value
  LirOperand index;       // index value (e.g. "0", "%idx")
  std::optional<LirNativeVectorAuthority> native_vector_authority;
  // Set only for the scalar-to-vector splat precursor lowering, whose
  // structured result/use/index facts are mandatory.
  bool requires_native_vector_authority = false;
};

struct LirExtractElementOp {
  LirOperand result;
  LirTypeRef vec_type;
  LirOperand vec;
  LirTypeRef index_type;
  LirOperand index;
  std::optional<LirNativeVectorAuthority> native_vector_authority;
};

struct LirShuffleVectorOp {
  LirOperand result;
  LirTypeRef vec_type;
  LirOperand vec1;
  LirOperand vec2;
  LirTypeRef mask_type;
  LirOperand mask;
  std::optional<LirNativeVectorAuthority> native_vector_authority;
  // Set only for the structured scalar-to-vector zero-initializer splat
  // lowering, whose native result/use facts are mandatory.
  bool requires_native_vector_authority = false;
};

// Typed va_arg instruction.
struct LirVaArgOp {
  LirOperand result;      // SSA name for result
  LirOperand ap_ptr;      // SSA name of va_list pointer
  LirTypeRef type_str;    // result type string
  bool requires_native_memory_va_authority = false;
  std::optional<LirMemoryVaPointerAuthority> ap_authority;
  std::optional<LirValueId> result_authority;
  std::optional<LirTypeRef> result_type_authority;
};

// Typed inline (non-hoisted) alloca instruction.
// Covers dynamic allocas (__builtin_alloca, alloca(), VLA) and temporary allocas
// emitted inline in the current block (va_list copy, variadic aggregate, vaarg struct).
struct LirAllocaOp {
  LirOperand result;      // SSA name for result
  LirTypeRef type_str;    // LLVM element type string (e.g. "i8", "i64", "%struct.foo")
  LirOperand count;       // dynamic count operand (empty for simple allocas)
  int align = 0;          // alignment in bytes (0 = unspecified)
  std::optional<LirCurrentFunctionLocalObjectPointer> local_object_authority;
};

// Typed inline asm call instruction.
struct LirInlineAsmInsnRMetadata {
  std::uint32_t opcode = 0;
  std::uint32_t funct3 = 0;
  std::uint32_t funct7 = 0;
  std::array<std::size_t, 3> operand_indices{};
};

enum class LirInlineAsmValueRole : std::uint8_t {
  Input,
  Output,
  ReadWrite,
};

// Constraint-position metadata for an ordinary typed-LIR value. This is not a
// second inline-asm value system: value remains the same LirOperand carrier
// used by other typed instructions, while role and constraint_index describe
// how that identity participates in this opaque operation.
struct LirInlineAsmValueBinding {
  LirOperand value;
  LirTypeRef type;
  LirInlineAsmValueRole role = LirInlineAsmValueRole::Input;
  std::size_t constraint_index = 0;
};

struct LirInlineAsmOp {
  // Non-authoritative LLVM compatibility rendering. Semantic consumers must
  // use original_* and the ordinary_inputs/ordinary_results bindings below.
  LirOperand result;          // rendered SSA result (empty for void asm)
  LirTypeRef ret_type;        // rendered LLVM return type
  std::string asm_text;       // rendered LLVM assembly template
  std::string constraints;    // rendered LLVM constraint string
  bool side_effects = false;  // semantic side-effect state; also rendered to LLVM
  std::string args_str;       // pre-formatted LLVM argument string

  // Semantic inline-asm authority. Text and clobbers remain opaque and
  // ordered; ordinary SSA identities carry all use/definition ownership.
  std::vector<std::string> clobbers;  // semantic ordered clobber names
  std::optional<LirInlineAsmInsnRMetadata> insn_r;
  std::string original_asm_text;
  std::string original_constraint_text;
  std::vector<LirInlineAsmValueBinding> ordinary_inputs;
  std::vector<LirInlineAsmValueBinding> ordinary_results;
};

using LirInst = std::variant<
    LirConstInt,
    LirConstFloat,
    LirLoad,
    LirStore,
    LirBinary,
    LirCast,
    LirCmp,
    LirCall,
    LirGep,
    LirSelect,
    LirIntrinsic,
    LirInlineAsm,
    LirMemcpyOp,
    LirVaStartOp,
    LirVaEndOp,
    LirVaCopyOp,
    LirStackSaveOp,
    LirStackRestoreOp,
    LirAbsOp,
    LirIndirectBrOp,
    LirExtractValueOp,
    LirInsertValueOp,
    LirLoadOp,
    LirStoreOp,
    LirMemsetOp,
    LirCastOp,
    LirGepOp,
    LirCallOp,
    LirBinOp,
    LirCmpOp,
    LirPhiOp,
    LirSelectOp,
    LirInsertElementOp,
    LirExtractElementOp,
    LirShuffleVectorOp,
    LirVaArgOp,
    LirAllocaOp,
    LirInlineAsmOp
>;

// ── Terminators ──────────────────────────────────────────────────────────────

struct LirBr {
  std::string target_label;  // e.g. "block_3"
  // Semantic CFG authority; the label remains a display shadow only.
  LirBlockId successor = LirBlockId::invalid();
};

struct LirCondBr {
  std::string cond_name;     // e.g. "%t5"
  std::string true_label;
  std::string false_label;
  // Semantic CFG authority; labels remain display shadows only.
  LirBlockId true_successor = LirBlockId::invalid();
  LirBlockId false_successor = LirBlockId::invalid();
  // Semantic condition authority; cond_name remains a checked display shadow.
  // Kept last so legacy aggregate producers retain their existing field order.
  LirValueId condition = LirValueId::invalid();
};

enum class LirNativeBodyParameterAbi : uint8_t;

enum class LirReturnValueParameterRole : uint8_t {
  Invalid,
  ReturnValue,
};

enum class LirSwitchSelectorParameterRole : uint8_t {
  Invalid,
  SwitchSelector,
};

// Native authority for exactly a direct plain scalar current-function
// parameter used unchanged as a switch selector. The selector spelling is a
// checked display mirror; selection facts live in this tuple.
struct LirSwitchSelectorParameterAuthority {
  LirValueId value = LirValueId::invalid();
  uint32_t parameter_index = 0;
  LirTypeRef type;
  LinkNameId owner = kInvalidLinkName;
  LirNativeBodyParameterAbi abi{};
  LirSwitchSelectorParameterRole role = LirSwitchSelectorParameterRole::Invalid;
};

// Native authority for exactly a direct plain scalar current-function
// parameter returned unchanged as an integer. The return operand remains a
// checked display mirror; it is never used to reconstruct this binding.
struct LirReturnValueParameterAuthority {
  LirValueId value = LirValueId::invalid();
  uint32_t parameter_index = 0;
  LirTypeRef type;
  LinkNameId owner = kInvalidLinkName;
  LirNativeBodyParameterAbi abi{};
  LirReturnValueParameterRole role = LirReturnValueParameterRole::Invalid;
};

struct LirRet {
  // Field spellings remain source-compatible with aggregate producers, but
  // their values now carry the same structured authority as instructions.
  std::optional<LirOperand> value_str;  // e.g. "%t7", "null", "0"
  LirTypeRef type_str;                  // e.g. "i32", "ptr", "void"
  std::optional<LirReturnValueParameterAuthority>
      return_value_parameter_authority;
};

struct LirSwitch {
  std::string selector_name;   // e.g. "%t3"
  std::string selector_type;   // e.g. "i32"
  std::string default_label;
  std::vector<std::pair<long long, std::string>> cases;  // {value, label}
  // Semantic CFG authority for default and case entries, in case order.
  LirBlockId default_successor = LirBlockId::invalid();
  std::vector<LirBlockId> case_successors;
  // Semantic selector authority; selector_name and selector_type are checked
  // display mirrors only. Kept last for aggregate-producer field order.
  LirValueId selector = LirValueId::invalid();
  // Structured selector type authority.  Keep this after selector so legacy
  // aggregate producers retain their existing field order.
  LirTypeRef selector_type_ref;
  // Present only when selector directly names a native direct-scalar
  // current-function parameter.
  std::optional<LirSwitchSelectorParameterAuthority> selector_parameter_authority;
};

struct LirIndirectBr {
  LirValueId addr{};
  std::vector<LirBlockId> targets;
};

struct LirUnreachable {};

using LirTerminator = std::variant<
    LirBr,
    LirCondBr,
    LirRet,
    LirSwitch,
    LirIndirectBr,
    LirUnreachable
>;

// ── Block ────────────────────────────────────────────────────────────────────

struct LirBlock {
  LirBlockId id{};
  std::string label;  // display label (e.g. ".LBB0")
  std::vector<LirInst> insts;
  LirTerminator terminator{LirUnreachable{}};
};

// ── Stack object (alloca) ────────────────────────────────────────────────────

struct LirStackObject {
  LirStackSlotId id{};
  std::string name;   // e.g. "lv.x"
  TypeSpec type{};
  int align = 0;
  bool is_vla = false;
};

// ── String constant ──────────────────────────────────────────────────────────

struct LirStringConst {
  std::string pool_name;   // e.g. "@.str.0"
  std::string raw_bytes;
  int byte_length = 0;
};

// ── External declaration ─────────────────────────────────────────────────────

struct LirExternDecl {
  std::string name;
  std::string return_type_str;  // LLVM return type (temporary; will be TypeSpec later)
  LirTypeRef return_type;
  LirExtAttr return_ext_attr = LirExtAttr::None;
  LinkNameId link_name_id = kInvalidLinkName;
  LirFunctionSignatureRef function_signature_ref =
      LirFunctionSignatureRef::invalid();
};

// ── Function ─────────────────────────────────────────────────────────────────

struct LirSignatureParam {
  std::string name;
  TypeSpec type{};
  bool is_byval = false;
};

// The sole pointer/object authority introduced for the selected fixed-aggregate
// byval materialization.  It is intentionally a pair, rather than a general
// pointer or object table: all other pointer/object families remain absent.
enum class LirSelectedMemcpyPointerRole : std::uint8_t {
  ByvalParameter,
  DestinationAlloca,
};

struct LirCurrentFunctionPointerDefinition {
  LirValueId value = LirValueId::invalid();
  LirTypeRef pointer_type;
  LirObjectId object = LirObjectId::invalid();
  LinkNameId object_owner = kInvalidLinkName;
  LirSelectedMemcpyPointerRole role =
      LirSelectedMemcpyPointerRole::ByvalParameter;
  bool live_at_selected_site = false;
};

struct LirSelectedMemcpyPointerAuthority {
  LirCurrentFunctionPointerDefinition byval_parameter;
  LirCurrentFunctionPointerDefinition destination_alloca;
};

// Native fixed pointer parameters that are directly usable by the current
// function body.  This is a definition carrier, not a spelling table: the
// parameter index, exact pointer type, owner, value identity, and ABI class
// must agree.
struct LirCurrentFunctionBodyParameterDefinition {
  LirValueId value = LirValueId::invalid();
  uint32_t parameter_index = 0;
  LirTypeRef type;
  LinkNameId owner = kInvalidLinkName;
  LirNativeBodyParameterAbi abi = LirNativeBodyParameterAbi::Invalid;
};

struct LirFunction {
  std::string name;
  LinkNameId link_name_id = kInvalidLinkName;
  bool is_internal = false;
  bool can_elide_if_unreferenced = false;
  bool is_declaration = false;  // true for declarations (no body)
  TypeSpec return_type{};
  std::vector<std::pair<std::string, TypeSpec>> params;  // name, type
  bool signature_is_variadic = false;
  bool signature_has_void_param_list = false;
  std::vector<LirSignatureParam> signature_params;
  std::optional<LirTypeRef> signature_return_type_ref;
  LirExtAttr signature_return_ext_attr = LirExtAttr::None;
  std::vector<LirTypeRef> signature_param_type_refs;
  LirFunctionSignatureRef function_signature_ref =
      LirFunctionSignatureRef::invalid();
  std::vector<LirBlock> blocks;
  std::vector<LirDirectLabelAddressConstant> direct_label_address_constants;
  std::vector<LirStackObject> stack_objects;
  LirBlockId entry{};

  // Optional prerequisite authority for exactly the selected byval parameter
  // materialization.  Containment in this function is the current-function
  // binding; each definition also carries the owner's stable LinkNameId so
  // foreign or stale bindings fail closed in the verifier.
  std::optional<LirSelectedMemcpyPointerAuthority>
      selected_memcpy_pointer_authority;

  std::vector<LirCurrentFunctionBodyParameterDefinition>
      native_body_parameter_definitions;

  // Final LLVM/output spelling for the function header plus legacy
  // no-metadata compatibility payload. Type identity mirrors live in
  // signature_return_type_ref, signature_params, and
  // signature_param_type_refs; backend/verifier code should prefer those
  // structured fields before consulting this text.
  // Function references embedded in this text have no structured producer
  // carrier yet, so any reachability scan of this field is an unresolved
  // producer-boundary compatibility fallback, not semantic authority.
  std::string signature_text;

  // Hoisted alloca instructions, rendered before entry block body instructions.
  std::vector<LirInst> alloca_insts;

  // ID generation for values and blocks within this function.
  uint32_t next_value_id = 0;
  uint32_t next_block_id = 0;
  uint32_t next_object_id = 0;

  [[nodiscard]] LirValueId alloc_value() { return LirValueId{next_value_id++}; }
  [[nodiscard]] LirBlockId alloc_block() { return LirBlockId{next_block_id++}; }
  [[nodiscard]] LirObjectId alloc_object() { return LirObjectId{next_object_id++}; }
};

// ── Global variable ──────────────────────────────────────────────────────────

// A global initializer element carries semantic authority independently of its
// final LLVM spelling.  Label addresses are function-scoped because LirBlockId
// values are only unique within their enclosing LirFunction.
struct LirGlobalInitializerLabelAddress {
  LinkNameId enclosing_function = kInvalidLinkName;
  LirBlockId target = LirBlockId::invalid();
};

using LirGlobalInitializerElement = std::variant<LirGlobalInitializerLabelAddress>;

struct LirGlobal {
  LirGlobalId id{};
  std::string name;        // Unquoted C name (printer quotes it)
  LinkNameId link_name_id = kInvalidLinkName;
  TypeSpec type{};
  bool is_internal = false;
  bool is_const = false;

  // Structured fields — lowering fills these, printer assembles LLVM text.
  std::string linkage_vis;  // e.g. "internal ", "external ", "weak ", "extern_weak ", ""
  std::string qualifier;    // "constant " or "global "
  std::string llvm_type;    // Final LLVM spelling for the global type
  std::optional<LirTypeRef> llvm_type_ref;  // Structured mirror when type identity is exact
  // Final LLVM spelling for the constant initializer (empty for extern decls).
  // initializer_function_link_name_ids carries semantic function references;
  // scanning init_text is retained only as a compatibility fallback for legacy
  // initializer producers that still emit raw LLVM payloads.
  std::string init_text;
  std::vector<LirGlobalInitializerElement> initializer_elements;
  std::vector<LinkNameId> initializer_function_link_name_ids;
  int align_bytes = 0;      // 0 = no align suffix
  bool is_extern_decl = false;  // extern: no init, type only
};

// ── Structured struct declaration mirror ────────────────────────────────────

struct LirStructField {
  LirTypeRef type;
};

struct LirStructDecl {
  StructNameId name_id = kInvalidStructName;
  std::vector<LirStructField> fields;
  bool is_packed = false;
  bool is_opaque = false;
};

// Module-owned aggregate store identity.  A reference is meaningful only in
// the LIR module that registered it; the HIR source ref records the canonical
// occurrence identity used to intern it.
struct LirAggregateRef {
  uint32_t value = std::numeric_limits<uint32_t>::max();
  [[nodiscard]] constexpr bool valid() const {
    return value != std::numeric_limits<uint32_t>::max();
  }
  [[nodiscard]] static constexpr LirAggregateRef invalid() {
    return LirAggregateRef{};
  }
};

// The declaration-lowering form retained by the module-owned aggregate store.
// Individual byte-storage fields still remain visible in `fields`; this kind
// distinguishes a normal declaration that happens to contain padding from an
// aggregate whose complete representation is byte storage.
enum class LirAggregateLayoutKind : uint8_t {
  Direct,
  ByteStorage,
  Union,
};

struct LirAggregateStoreEntry {
  c4c::hir::HirAggregateRef hir_ref;
  StructNameId name_id = kInvalidStructName;
  bool is_union = false;
  std::vector<LirStructField> fields;
  bool is_packed = false;
  bool is_opaque = false;
  LirAggregateLayoutKind layout_kind = LirAggregateLayoutKind::Direct;
};

struct LirFunctionSignatureStoreEntry {
  std::optional<LirTypeRef> return_type_ref;
  LirExtAttr return_ext_attr = LirExtAttr::None;
  std::vector<LirTypeRef> fixed_param_type_refs;
  std::vector<bool> fixed_param_is_byval;
  bool is_variadic = false;
  bool has_void_param_list = false;
};

struct LirStructuredLayoutObservation {
  std::string site;
  std::string type_name;
  StructNameId name_id = kInvalidStructName;
  bool legacy_found = false;
  bool structured_found = false;
  bool parity_checked = false;
  bool parity_matches = false;
  int legacy_size_bytes = 0;
  int legacy_align_bytes = 0;
  std::vector<std::string> legacy_field_types;
  std::vector<std::string> structured_field_types;
};

// ── Specialization metadata entry ────────────────────────────────────────────

struct LirSpecEntry {
  std::string spec_key;
  std::string template_origin;
  std::string mangled_name;
  LinkNameId mangled_link_name_id = kInvalidLinkName;
};

// ── Module ───────────────────────────────────────────────────────────────────

struct LirModule {
  struct ExternDeclInfo {
    std::string name;
    std::string return_type_str;
    LirTypeRef return_type;
    LirExtAttr return_ext_attr = LirExtAttr::None;
    LinkNameId link_name_id = kInvalidLinkName;
    LirFunctionSignatureRef function_signature_ref =
        LirFunctionSignatureRef::invalid();
  };

  c4c::TargetProfile target_profile{};
  std::string data_layout;
  std::shared_ptr<c4c::TextTable> link_name_texts;
  std::shared_ptr<std::deque<std::string>> type_tag_storage;
  c4c::LinkNameTable link_names;
  c4c::StructNameTable struct_names;

  [[nodiscard]] const char* intern_type_tag(const char* tag) {
    if (!tag || !tag[0]) return tag;
    if (!type_tag_storage) {
      type_tag_storage = std::make_shared<std::deque<std::string>>();
    }
    type_tag_storage->emplace_back(tag);
    return type_tag_storage->back().c_str();
  }

  std::vector<LirGlobal> globals;
  std::vector<LirFunction> functions;
  std::vector<LirStringConst> string_pool;
  std::vector<LirExternDecl> extern_decls;

  // Native SSA value identities are module-wide: verifier ownership is keyed
  // by the numeric LirValueId, so lowering must not restart this namespace for
  // every LirFunction.  LirFunction retains its allocator for standalone
  // construction compatibility; normal module lowering uses this allocator.
  uint32_t next_value_id = 0;

  [[nodiscard]] LirValueId alloc_value() { return LirValueId{next_value_id++}; }

  // Extern call declarations dedup first by semantic link-visible identity.
  // The raw/rendered-name map is a legacy compatibility and output boundary
  // for declarations that arrive before complete LinkNameId metadata exists.
  std::unordered_map<LinkNameId, ExternDeclInfo> extern_decl_link_name_map;
  std::unordered_map<std::string, ExternDeclInfo> extern_decl_name_map;

  [[nodiscard]] LirTypeRef extern_return_type_ref(const std::string& ret_ty) const {
    const StructNameId struct_name_id = struct_names.find(ret_ty);
    if (struct_name_id != kInvalidStructName && find_struct_decl(struct_name_id)) {
      return LirTypeRef::struct_type(ret_ty, struct_name_id);
    }
    return LirTypeRef::stored_extern_declaration_return_text(ret_ty);
  }

  void merge_extern_decl_info(ExternDeclInfo& info, const std::string& name,
                              const std::string& ret_ty,
                              const LirTypeRef& ret_type,
                              LirExtAttr return_ext_attr,
                              LinkNameId link_name_id) {
    if (info.name.empty()) info.name = name;
    if (info.return_type_str == "void" && ret_ty != "void") {
      info.return_type_str = ret_ty;
      info.return_type = ret_type;
      info.return_ext_attr = return_ext_attr;
    } else if (info.return_type_str == ret_ty &&
               !info.return_type.has_struct_name_id() &&
               ret_type.has_struct_name_id()) {
      info.return_type = ret_type;
    }
    if (info.return_ext_attr == LirExtAttr::None &&
        return_ext_attr != LirExtAttr::None) {
      info.return_ext_attr = return_ext_attr;
    }
    if (info.link_name_id == kInvalidLinkName &&
        link_name_id != kInvalidLinkName) {
      info.link_name_id = link_name_id;
    }
  }

  /// Record an extern function call declaration. Prefer semantic dedup by
  /// LinkNameId and fall back to legacy raw/rendered-name dedup only when no
  /// semantic id exists yet. Upgrades void returns to concrete types when a
  /// non-void call is seen.
  void record_extern_decl(const std::string& name, const std::string& ret_ty,
                          LinkNameId link_name_id = kInvalidLinkName,
                          LirExtAttr return_ext_attr = LirExtAttr::None) {
    if (link_name_id == kInvalidLinkName) {
      const LinkNameId known_link_name_id = link_names.find(name);
      if (known_link_name_id != kInvalidLinkName &&
          extern_decl_link_name_map.find(known_link_name_id) !=
              extern_decl_link_name_map.end()) {
        link_name_id = known_link_name_id;
      }
    }

    const LirTypeRef ret_type = extern_return_type_ref(ret_ty);
    if (link_name_id != kInvalidLinkName) {
      auto it = extern_decl_link_name_map.find(link_name_id);
      if (it == extern_decl_link_name_map.end()) {
        auto by_name = extern_decl_name_map.find(name);
        if (by_name != extern_decl_name_map.end()) {
          ExternDeclInfo info = std::move(by_name->second);
          extern_decl_name_map.erase(by_name);
          merge_extern_decl_info(info, name, ret_ty, ret_type, return_ext_attr,
                                 link_name_id);
          extern_decl_link_name_map.emplace(link_name_id, std::move(info));
          return;
        }
        extern_decl_link_name_map.emplace(
            link_name_id,
            ExternDeclInfo{name, ret_ty, ret_type, return_ext_attr, link_name_id});
        return;
      }
      merge_extern_decl_info(it->second, name, ret_ty, ret_type,
                             return_ext_attr, link_name_id);
      return;
    }

    auto it = extern_decl_name_map.find(name);
    if (it == extern_decl_name_map.end()) {
      extern_decl_name_map.emplace(
          name, ExternDeclInfo{name, ret_ty, ret_type, return_ext_attr, link_name_id});
      return;
    }
    merge_extern_decl_info(it->second, name, ret_ty, ret_type,
                           return_ext_attr, link_name_id);
  }

  // Legacy type declaration shadow kept for compatibility with older output
  // producers and verifier parity checks. When `struct_decls` is populated,
  // structured declarations are the printer/backend authority.
  std::vector<std::string> type_decls;

  // Structured struct declarations used by the printer and backend layout
  // path. Matching `type_decls` lines are retained only as legacy shadows.
  std::vector<LirStructDecl> struct_decls;
  std::unordered_map<StructNameId, std::size_t> struct_decl_index;
  mutable std::vector<LirStructuredLayoutObservation> structured_layout_observations;

  // The canonical aggregate seam is intentionally separate from struct-name
  // and rendered declaration compatibility indexes.
  std::vector<LirAggregateStoreEntry> aggregate_store;
  std::unordered_map<uint64_t, LirAggregateRef> aggregate_ref_by_hir_ref;
  std::vector<LirFunctionSignatureStoreEntry> function_signature_store;
  std::vector<LirVectorStoreEntry> vector_store;

  [[nodiscard]] static uint64_t aggregate_store_key(c4c::hir::HirAggregateRef ref) {
    return (static_cast<uint64_t>(ref.module.value) << 32) | ref.aggregate.value;
  }

  [[nodiscard]] LirAggregateRef find_aggregate_ref(const c4c::hir::Module& source_module,
                                                    c4c::hir::HirAggregateRef ref) const {
    if (!source_module.owns_aggregate_ref(ref)) return LirAggregateRef::invalid();
    const auto it = aggregate_ref_by_hir_ref.find(aggregate_store_key(ref));
    return it == aggregate_ref_by_hir_ref.end() ? LirAggregateRef::invalid() : it->second;
  }

  [[nodiscard]] const LirAggregateStoreEntry* find_aggregate(
      LirAggregateRef ref) const {
    return ref.valid() && ref.value < aggregate_store.size() ? &aggregate_store[ref.value]
                                                               : nullptr;
  }

  [[nodiscard]] const LirVectorStoreEntry* find_vector(LirVectorRef ref) const {
    return ref.valid() && ref.value < vector_store.size() ? &vector_store[ref.value]
                                                          : nullptr;
  }

  [[nodiscard]] LirVectorRef register_vector(LirVectorStoreEntry entry) {
    if (entry.lane_count == 0 || entry.element_type.empty()) {
      throw std::runtime_error("cannot register an empty LIR vector fact");
    }
    for (std::size_t index = 0; index < vector_store.size(); ++index) {
      const LirVectorStoreEntry& existing = vector_store[index];
      if (existing.lane_count == entry.lane_count &&
          existing.element_type == entry.element_type &&
          existing.element_type.str() == entry.element_type.str()) {
        return LirVectorRef{static_cast<uint32_t>(index)};
      }
    }
    const LirVectorRef ref{static_cast<uint32_t>(vector_store.size())};
    vector_store.push_back(std::move(entry));
    return ref;
  }

  [[nodiscard]] LirAggregateRef register_aggregate(const c4c::hir::Module& source_module,
                                                    c4c::hir::HirAggregateRef hir_ref,
                                                    StructNameId name_id,
                                                    bool is_union) {
    if (!source_module.owns_aggregate_ref(hir_ref)) {
      throw std::runtime_error(
          "cannot register an invalid, incomplete, or foreign HIR aggregate ref");
    }
    if (name_id == kInvalidStructName) {
      throw std::runtime_error("cannot register an aggregate without a LIR declaration name");
    }
    if (const LirAggregateRef existing = find_aggregate_ref(source_module, hir_ref);
        existing.valid()) {
      return existing;
    }
    const LirAggregateRef ref{static_cast<uint32_t>(aggregate_store.size())};
    aggregate_store.push_back({hir_ref, name_id, is_union});
    aggregate_ref_by_hir_ref.emplace(aggregate_store_key(hir_ref), ref);
    return ref;
  }

  void record_aggregate_decl_facts(LirAggregateRef ref, const LirStructDecl& decl,
                                   LirAggregateLayoutKind layout_kind) {
    if (!ref.valid() || ref.value >= aggregate_store.size() ||
        aggregate_store[ref.value].name_id != decl.name_id) {
      throw std::runtime_error("cannot record aggregate facts for a mismatched LIR declaration");
    }
    LirAggregateStoreEntry* entry = &aggregate_store[ref.value];
    entry->fields = decl.fields;
    entry->is_packed = decl.is_packed;
    entry->is_opaque = decl.is_opaque;
    entry->layout_kind = layout_kind;
  }

  [[nodiscard]] static bool same_function_signature_entry(
      const LirFunctionSignatureStoreEntry& lhs,
      const LirFunctionSignatureStoreEntry& rhs) {
    const auto same_type = [](const LirTypeRef& a, const LirTypeRef& b) {
      return a == b && a.str() == b.str();
    };
    const auto same_optional_type =
        [&](const std::optional<LirTypeRef>& a,
            const std::optional<LirTypeRef>& b) {
          if (a.has_value() != b.has_value()) return false;
          return !a.has_value() || same_type(*a, *b);
        };
    if (!same_optional_type(lhs.return_type_ref, rhs.return_type_ref) ||
        lhs.return_ext_attr != rhs.return_ext_attr ||
        lhs.fixed_param_type_refs.size() != rhs.fixed_param_type_refs.size() ||
        lhs.fixed_param_is_byval != rhs.fixed_param_is_byval ||
        lhs.is_variadic != rhs.is_variadic ||
        lhs.has_void_param_list != rhs.has_void_param_list) {
      return false;
    }
    for (std::size_t index = 0; index < lhs.fixed_param_type_refs.size();
         ++index) {
      if (!same_type(lhs.fixed_param_type_refs[index],
                     rhs.fixed_param_type_refs[index])) {
        return false;
      }
    }
    return true;
  }

  [[nodiscard]] LirFunctionSignatureRef register_function_signature(
      LirFunctionSignatureStoreEntry entry) {
    if (entry.has_void_param_list &&
        (!entry.fixed_param_type_refs.empty() || entry.is_variadic)) {
      throw std::runtime_error(
          "void function-signature parameter list cannot carry fixed params or variadic state");
    }
    if (entry.fixed_param_is_byval.size() != entry.fixed_param_type_refs.size()) {
      throw std::runtime_error(
          "function-signature byval facts must match fixed parameter refs");
    }
    for (std::size_t index = 0; index < function_signature_store.size(); ++index) {
      if (same_function_signature_entry(function_signature_store[index], entry)) {
        return LirFunctionSignatureRef{static_cast<uint32_t>(index)};
      }
    }
    const LirFunctionSignatureRef ref{
        static_cast<uint32_t>(function_signature_store.size())};
    function_signature_store.push_back(std::move(entry));
    return ref;
  }

  [[nodiscard]] const LirFunctionSignatureStoreEntry* find_function_signature(
      LirFunctionSignatureRef ref) const {
    return ref.valid() && ref.value < function_signature_store.size()
               ? &function_signature_store[ref.value]
               : nullptr;
  }

  [[nodiscard]] LirFunctionSignatureRef register_extern_function_signature(
      const std::string& name, LinkNameId link_name_id,
      const LirCallSignature& signature) {
    if (!signature.return_type_ref.has_value() ||
        signature.has_unspecified_params ||
        signature.fixed_param_types.size() !=
            signature.fixed_param_type_refs.size()) {
      return LirFunctionSignatureRef::invalid();
    }
    LirFunctionSignatureStoreEntry entry;
    entry.return_type_ref = signature.return_type_ref;
    entry.return_ext_attr = signature.return_ext_attr;
    entry.fixed_param_type_refs = signature.fixed_param_type_refs;
    entry.fixed_param_is_byval.assign(entry.fixed_param_type_refs.size(), false);
    entry.is_variadic = signature.is_variadic;
    entry.has_void_param_list = signature.has_void_param_list;
    const LirFunctionSignatureRef ref =
        register_function_signature(std::move(entry));

    if (link_name_id != kInvalidLinkName) {
      auto it = extern_decl_link_name_map.find(link_name_id);
      if (it != extern_decl_link_name_map.end()) {
        it->second.function_signature_ref = ref;
      }
      return ref;
    }
    auto it = extern_decl_name_map.find(name);
    if (it != extern_decl_name_map.end()) {
      it->second.function_signature_ref = ref;
    }
    return ref;
  }

  LirStructDecl* find_struct_decl(StructNameId name_id) {
    const auto it = struct_decl_index.find(name_id);
    if (it == struct_decl_index.end()) return nullptr;
    return &struct_decls[it->second];
  }

  const LirStructDecl* find_struct_decl(StructNameId name_id) const {
    const auto it = struct_decl_index.find(name_id);
    if (it == struct_decl_index.end()) return nullptr;
    return &struct_decls[it->second];
  }

  LirStructDecl& record_struct_decl(LirStructDecl decl) {
    const StructNameId name_id = decl.name_id;
    if (name_id != kInvalidStructName) {
      LirStructDecl* existing = find_struct_decl(name_id);
      if (existing) {
        *existing = std::move(decl);
        return *existing;
      }
    }
    const std::size_t index = struct_decls.size();
    struct_decls.push_back(std::move(decl));
    if (name_id != kInvalidStructName) {
      struct_decl_index.emplace(name_id, index);
    }
    return struct_decls.back();
  }

  // Intrinsic requirement flags — set directly by lowering (emit_stmt).
  bool need_va_start = false;
  bool need_va_end = false;
  bool need_va_copy = false;
  bool need_memcpy = false;
  bool need_memset = false;
  bool need_stacksave = false;
  bool need_stackrestore = false;
  bool need_abs = false;
  bool need_ptrmask = false;
  bool prefer_semantic_va_ops = false;

  // Specialization metadata for cross-TU serialization.
  std::vector<LirSpecEntry> spec_entries;

  // ── String constant pool ──────────────────────────────────────────────────
  // Dedup map: raw bytes → pool name (e.g. "@.str0").
  std::unordered_map<std::string, std::string> str_pool_map;
  int str_pool_idx = 0;

  /// Intern a string literal.  Returns the pool name (e.g. "@.str3").
  /// Deduplicates by raw byte content and appends to string_pool on first use.
  std::string intern_str(const std::string& raw_bytes) {
    auto it = str_pool_map.find(raw_bytes);
    if (it != str_pool_map.end()) return it->second;
    const std::string name = "@.str" + std::to_string(str_pool_idx++);
    str_pool_map[raw_bytes] = name;
    const size_t len = raw_bytes.size() + 1;
    std::string esc;
    for (unsigned char c : raw_bytes) {
      if (c == '"')       { esc += "\\22"; }
      else if (c == '\\') { esc += "\\5C"; }
      else if (c == '\n') { esc += "\\0A"; }
      else if (c == '\r') { esc += "\\0D"; }
      else if (c == '\t') { esc += "\\09"; }
      else if (c < 32 || c >= 127) {
        char buf[8]; std::snprintf(buf, sizeof(buf), "\\%02X", c); esc += buf;
      } else {
        esc += static_cast<char>(c);
      }
    }
    LirStringConst sc;
    sc.pool_name = name;
    sc.raw_bytes = esc;
    sc.byte_length = static_cast<int>(len);
    string_pool.push_back(std::move(sc));
    return name;
  }
};

// LIR verification and rendering live on the public IR surface. Keep these
// declarations here so agents can use `ir.hpp` as the LIR index entry.
enum class LirVerifyErrorKind {
  Malformed,
};

class LirVerifyError : public std::invalid_argument {
 public:
  LirVerifyError(LirVerifyErrorKind kind, const std::string& message)
      : std::invalid_argument(message), kind_(kind) {}

  [[nodiscard]] LirVerifyErrorKind kind() const noexcept { return kind_; }

 private:
  LirVerifyErrorKind kind_;
};

const std::string& require_operand_kind(
    const LirOperand& operand,
    std::string_view field,
    std::initializer_list<LirOperandKind> allowed_kinds,
    bool allow_empty = false);

const std::string& require_type_ref(const LirTypeRef& type,
                                    std::string_view field,
                                    bool allow_void = false);

std::string_view render_binary_opcode(const LirBinaryOpcodeRef& opcode,
                                      std::string_view field);

std::string_view render_cmp_predicate(const LirCmpPredicateRef& predicate,
                                      std::string_view field);

std::string render_struct_decl_llvm(const LirModule& mod,
                                    const LirStructDecl& decl);

void verify_module(const LirModule& mod);

std::string print_llvm(const LirModule& mod);

}  // namespace c4c::codegen::lir
