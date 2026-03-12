#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "ast.hpp"

namespace tinyc2ll::frontend_cxx::sema_ir {

// This header is the migration IR contract for docs/sema_ir_split_plan.md.
//
// Phase mapping:
// - Phase 2: phase2::hir::* (typed frontend IR, AST lowering target)
// - Phase 3: backend-facing hooks embedded in HIR (attrs/debug/meta)
// - Phase 4: phase4::dag::* (lowering skeleton + inspectable DAG)
// - Phase 5+: stable IDs, summaries, and debug dumps for parity tooling

using SymbolName = std::string;

struct SourceLoc {
  int line = 0;
};

struct SourceSpan {
  SourceLoc begin{};
  SourceLoc end{};
};

struct Linkage {
  bool is_static = false;
  bool is_extern = false;
  bool is_inline = false;
};

enum class StorageClass : uint8_t {
  Auto,
  Register,
  Static,
  Extern,
  TypedefLike,
};

enum class ValueCategory : uint8_t {
  RValue,
  LValue,
};

struct QualType {
  TypeSpec spec{};
  ValueCategory category = ValueCategory::RValue;
  bool is_const_expr = false;
};

struct FnAttr {
  bool variadic = false;
  bool no_return = false;
  bool no_inline = false;
  bool always_inline = false;
};

struct FnPtrSig {
  QualType return_type{};
  std::vector<QualType> params;
  bool variadic = false;
  bool unspecified_params = false;
};

struct Param {
  SymbolName name;
  QualType type;
  std::optional<FnPtrSig> fn_ptr_sig;
  SourceSpan span{};
};

struct LocalId {
  uint32_t value = 0;

  static constexpr uint32_t kInvalid = std::numeric_limits<uint32_t>::max();

  [[nodiscard]] constexpr bool valid() const { return value != kInvalid; }
  [[nodiscard]] constexpr size_t index() const { return static_cast<size_t>(value); }
  [[nodiscard]] static constexpr LocalId invalid() { return LocalId{kInvalid}; }
};

struct BlockId {
  uint32_t value = 0;

  static constexpr uint32_t kInvalid = std::numeric_limits<uint32_t>::max();

  [[nodiscard]] constexpr bool valid() const { return value != kInvalid; }
  [[nodiscard]] constexpr size_t index() const { return static_cast<size_t>(value); }
  [[nodiscard]] static constexpr BlockId invalid() { return BlockId{kInvalid}; }
  [[nodiscard]] std::string as_label() const { return ".LBB" + std::to_string(value); }
};

struct ExprId {
  uint32_t value = 0;

  static constexpr uint32_t kInvalid = std::numeric_limits<uint32_t>::max();

  [[nodiscard]] constexpr bool valid() const { return value != kInvalid; }
  [[nodiscard]] constexpr size_t index() const { return static_cast<size_t>(value); }
  [[nodiscard]] static constexpr ExprId invalid() { return ExprId{kInvalid}; }
};

struct GlobalId {
  uint32_t value = 0;

  static constexpr uint32_t kInvalid = std::numeric_limits<uint32_t>::max();

  [[nodiscard]] constexpr bool valid() const { return value != kInvalid; }
  [[nodiscard]] constexpr size_t index() const { return static_cast<size_t>(value); }
  [[nodiscard]] static constexpr GlobalId invalid() { return GlobalId{kInvalid}; }
};

struct FunctionId {
  uint32_t value = 0;

  static constexpr uint32_t kInvalid = std::numeric_limits<uint32_t>::max();

  [[nodiscard]] constexpr bool valid() const { return value != kInvalid; }
  [[nodiscard]] constexpr size_t index() const { return static_cast<size_t>(value); }
  [[nodiscard]] static constexpr FunctionId invalid() { return FunctionId{kInvalid}; }
};

struct IntLiteral {
  long long value = 0;
  bool is_unsigned = false;
};

struct FloatLiteral {
  double value = 0.0;
};

struct StringLiteral {
  std::string raw;
  bool is_wide = false;
};

struct LabelAddrExpr {
  SymbolName label_name;  // GCC &&label extension
  SymbolName fn_name;     // enclosing function (for blockaddress)
};

struct CharLiteral {
  long long value = 0;
};

struct DeclRef {
  SymbolName name;
  std::optional<LocalId> local;
  std::optional<uint32_t> param_index;
  std::optional<GlobalId> global;
};

enum class UnaryOp : uint8_t {
  Plus,
  Minus,
  Not,
  BitNot,
  AddrOf,
  Deref,
  PreInc,
  PreDec,
  PostInc,
  PostDec,
  RealPart,
  ImagPart,
};

enum class BinaryOp : uint8_t {
  Add,
  Sub,
  Mul,
  Div,
  Mod,
  Shl,
  Shr,
  BitAnd,
  BitOr,
  BitXor,
  Lt,
  Le,
  Gt,
  Ge,
  Eq,
  Ne,
  LAnd,
  LOr,
  Comma,
};

enum class AssignOp : uint8_t {
  Set,
  Add,
  Sub,
  Mul,
  Div,
  Mod,
  Shl,
  Shr,
  BitAnd,
  BitOr,
  BitXor,
};

struct Expr;

struct UnaryExpr {
  UnaryOp op = UnaryOp::Plus;
  ExprId operand{};
};

struct BinaryExpr {
  BinaryOp op = BinaryOp::Add;
  ExprId lhs{};
  ExprId rhs{};
};

struct AssignExpr {
  AssignOp op = AssignOp::Set;
  ExprId lhs{};
  ExprId rhs{};
};

struct CastExpr {
  QualType to_type{};
  ExprId expr{};
};

struct CallExpr {
  ExprId callee{};
  std::vector<ExprId> args;
  BuiltinId builtin_id = BuiltinId::Unknown;
};

struct VaArgExpr {
  ExprId ap{};
};

struct IndexExpr {
  ExprId base{};
  ExprId index{};
};

struct MemberExpr {
  ExprId base{};
  SymbolName field;
  bool is_arrow = false;
};

struct TernaryExpr {
  ExprId cond{};
  ExprId then_expr{};
  ExprId else_expr{};
};

struct SizeofExpr {
  ExprId expr{};
};

struct SizeofTypeExpr {
  QualType type{};
};

using ExprPayload = std::variant<
    IntLiteral,
    FloatLiteral,
    StringLiteral,
    CharLiteral,
    DeclRef,
    UnaryExpr,
    BinaryExpr,
    AssignExpr,
    CastExpr,
    CallExpr,
    VaArgExpr,
    IndexExpr,
    MemberExpr,
    TernaryExpr,
    SizeofExpr,
    SizeofTypeExpr,
    LabelAddrExpr>;

struct Expr {
  ExprId id{};
  QualType type{};
  SourceSpan span{};
  ExprPayload payload{};
};

struct LocalDecl {
  LocalId id{};
  SymbolName name;
  QualType type{};
  std::optional<FnPtrSig> fn_ptr_sig;
  StorageClass storage = StorageClass::Auto;
  std::optional<ExprId> vla_size;
  std::optional<ExprId> init;
  SourceSpan span{};
};

struct LabelRef {
  SymbolName user_name;
  BlockId resolved_block = BlockId::invalid();
};

struct CaseRange {
  long long lo = 0;
  long long hi = 0;
};

struct Stmt;

struct ExprStmt {
  std::optional<ExprId> expr;
};

struct ReturnStmt {
  std::optional<ExprId> expr;
};

struct IfStmt {
  ExprId cond{};
  BlockId then_block{};
  std::optional<BlockId> else_block;
  BlockId after_block{};  // block that follows both branches (join point)
};

struct WhileStmt {
  ExprId cond{};
  BlockId body_block{};
  std::optional<BlockId> continue_target;
  std::optional<BlockId> break_target;
};

struct ForStmt {
  std::optional<ExprId> init;
  std::optional<ExprId> cond;
  std::optional<ExprId> update;
  BlockId body_block{};
  std::optional<BlockId> continue_target;
  std::optional<BlockId> break_target;
};

struct DoWhileStmt {
  BlockId body_block{};
  ExprId cond{};
  std::optional<BlockId> continue_target;
  std::optional<BlockId> break_target;
};

struct SwitchStmt {
  ExprId cond{};
  BlockId body_block{};
  std::optional<BlockId> default_block;
  std::optional<BlockId> break_block;  // after-switch block (break target)
  std::vector<std::pair<long long, BlockId>> case_blocks;  // case value → block
  std::vector<std::tuple<long long, long long, BlockId>> case_range_blocks;  // lo, hi, block
};

struct GotoStmt {
  LabelRef target{};
};

struct IndirBrStmt {
  ExprId target{};  // GCC computed goto: goto *expr
};

struct LabelStmt {
  SymbolName name;
};

struct CaseStmt {
  long long value = 0;
};

struct CaseRangeStmt {
  CaseRange range{};
};

struct DefaultStmt {};
struct BreakStmt {
  std::optional<BlockId> target;
};
struct ContinueStmt {
  std::optional<BlockId> target;
};

using StmtPayload = std::variant<
    LocalDecl,
    ExprStmt,
    ReturnStmt,
    IfStmt,
    WhileStmt,
    ForStmt,
    DoWhileStmt,
    SwitchStmt,
    GotoStmt,
    IndirBrStmt,
    LabelStmt,
    CaseStmt,
    CaseRangeStmt,
    DefaultStmt,
    BreakStmt,
    ContinueStmt>;

struct Stmt {
  StmtPayload payload{};
  SourceSpan span{};

  void append_successors(std::vector<BlockId>& out) const {
    auto push_unique = [&out](BlockId id) {
      if (!id.valid()) return;
      const auto it = std::find_if(
          out.begin(), out.end(), [&](const BlockId& b) { return b.value == id.value; });
      if (it == out.end()) out.push_back(id);
    };
    std::visit(
        [&](const auto& s) {
          using T = std::decay_t<decltype(s)>;
          if constexpr (std::is_same_v<T, IfStmt>) {
            push_unique(s.then_block);
            if (s.else_block) push_unique(*s.else_block);
            push_unique(s.after_block);
          } else if constexpr (std::is_same_v<T, WhileStmt>) {
            push_unique(s.body_block);
            if (s.continue_target) push_unique(*s.continue_target);
            if (s.break_target) push_unique(*s.break_target);
          } else if constexpr (std::is_same_v<T, ForStmt>) {
            push_unique(s.body_block);
            if (s.continue_target) push_unique(*s.continue_target);
            if (s.break_target) push_unique(*s.break_target);
          } else if constexpr (std::is_same_v<T, DoWhileStmt>) {
            push_unique(s.body_block);
            if (s.continue_target) push_unique(*s.continue_target);
            if (s.break_target) push_unique(*s.break_target);
          } else if constexpr (std::is_same_v<T, SwitchStmt>) {
            push_unique(s.body_block);
            if (s.default_block) push_unique(*s.default_block);
            for (const auto& [val, bid] : s.case_blocks) push_unique(bid);
          } else if constexpr (std::is_same_v<T, GotoStmt>) {
            push_unique(s.target.resolved_block);
          } else if constexpr (std::is_same_v<T, BreakStmt>) {
            if (s.target) push_unique(*s.target);
          } else if constexpr (std::is_same_v<T, ContinueStmt>) {
            if (s.target) push_unique(*s.target);
          }
        },
        payload);
  }
};

struct Block {
  BlockId id{};
  std::vector<Stmt> stmts;
  bool has_explicit_terminator = false;

  std::vector<BlockId> successor_blocks() const {
    std::vector<BlockId> out;
    for (const auto& stmt : stmts) {
      stmt.append_successors(out);
    }
    return out;
  }
};

struct Function {
  FunctionId id{};
  SymbolName name;
  QualType return_type{};
  std::vector<Param> params;
  FnAttr attrs{};
  Linkage linkage{};
  std::vector<Block> blocks;
  BlockId entry{};
  SourceSpan span{};

  Block* find_block(BlockId id) {
    auto it = std::find_if(
        blocks.begin(), blocks.end(), [&](const Block& bb) { return bb.id.value == id.value; });
    return it == blocks.end() ? nullptr : &(*it);
  }

  const Block* find_block(BlockId id) const {
    auto it = std::find_if(
        blocks.begin(), blocks.end(), [&](const Block& bb) { return bb.id.value == id.value; });
    return it == blocks.end() ? nullptr : &(*it);
  }
};

struct InitScalar {
  ExprId expr{};
};

struct InitList;

struct InitListItem {
  std::optional<long long> index_designator;
  std::optional<SymbolName> field_designator;
  std::variant<InitScalar, std::shared_ptr<InitList>> value;
};

struct InitList {
  std::vector<InitListItem> items;
};

using GlobalInit = std::variant<std::monostate, InitScalar, InitList>;

struct GlobalVar {
  GlobalId id{};
  SymbolName name;
  QualType type{};
  std::optional<FnPtrSig> fn_ptr_sig;
  Linkage linkage{};
  bool is_const = false;
  GlobalInit init{};
  SourceSpan span{};
};

struct ProgramSummary {
  size_t functions = 0;
  size_t globals = 0;
  size_t blocks = 0;
  size_t statements = 0;
  size_t expressions = 0;

  ProgramSummary& operator+=(const ProgramSummary& rhs) {
    functions += rhs.functions;
    globals += rhs.globals;
    blocks += rhs.blocks;
    statements += rhs.statements;
    expressions += rhs.expressions;
    return *this;
  }
};

// ── Struct/union layout metadata (populated by ast_to_hir) ───────────────────

struct HirStructField {
  SymbolName name;
  TypeSpec elem_type;           // element type (first array dim cleared into array_first_dim)
  long long array_first_dim = -1;  // >=0 if field is an array type
  int llvm_idx = 0;             // LLVM struct field index
  bool is_anon_member = false;  // anonymous struct/union embedded member
  bool is_flexible_array = false;
  int offset_bytes = 0;
  int size_bytes = 0;
  int align_bytes = 1;
  // Bitfield metadata (-1 = not a bitfield)
  int bit_width = -1;           // width in bits (0 for zero-width padding bitfield)
  int bit_offset = 0;           // offset within storage unit (from LSB)
  int storage_unit_bits = 0;    // backing storage integer size (8, 16, 32, or 64)
  bool is_bf_signed = false;    // true if original declared type was signed
};

struct HirStructDef {
  SymbolName tag;
  bool is_union = false;
  int size_bytes = 0;
  int align_bytes = 1;
  std::vector<HirStructField> fields;
};

namespace phase2::hir {

struct Module {
  std::vector<Function> functions;
  std::vector<GlobalVar> globals;
  std::vector<Expr> expr_pool;

  std::unordered_map<SymbolName, FunctionId> fn_index;
  std::unordered_map<SymbolName, GlobalId> global_index;

  // Struct/union definitions (populated by lower_ast_to_hir)
  std::unordered_map<SymbolName, HirStructDef> struct_defs;
  std::vector<SymbolName> struct_def_order;  // insertion order for deterministic emission

  // Stable ID cursors for multi-pass transforms that may append nodes.
  uint32_t next_function_id = 0;
  uint32_t next_global_id = 0;
  uint32_t next_local_id = 0;
  uint32_t next_block_id = 0;
  uint32_t next_expr_id = 0;

  [[nodiscard]] FunctionId alloc_function_id() { return FunctionId{next_function_id++}; }
  [[nodiscard]] GlobalId alloc_global_id() { return GlobalId{next_global_id++}; }
  [[nodiscard]] LocalId alloc_local_id() { return LocalId{next_local_id++}; }
  [[nodiscard]] BlockId alloc_block_id() { return BlockId{next_block_id++}; }
  [[nodiscard]] ExprId alloc_expr_id() { return ExprId{next_expr_id++}; }

  void sync_next_ids_from_contents() {
    uint32_t max_fn = 0;
    uint32_t max_global = 0;
    uint32_t max_local = 0;
    uint32_t max_block = 0;
    uint32_t max_expr = 0;
    for (const auto& fn : functions) {
      if (fn.id.valid()) max_fn = std::max(max_fn, fn.id.value + 1);
      for (const auto& bb : fn.blocks) {
        if (bb.id.valid()) max_block = std::max(max_block, bb.id.value + 1);
        for (const auto& stmt : bb.stmts) {
          if (const auto* decl = std::get_if<LocalDecl>(&stmt.payload)) {
            if (decl->id.valid()) max_local = std::max(max_local, decl->id.value + 1);
          }
        }
      }
    }
    for (const auto& gv : globals)
      if (gv.id.valid()) max_global = std::max(max_global, gv.id.value + 1);
    for (const auto& expr : expr_pool)
      if (expr.id.valid()) max_expr = std::max(max_expr, expr.id.value + 1);
    next_function_id = std::max(next_function_id, max_fn);
    next_global_id = std::max(next_global_id, max_global);
    next_local_id = std::max(next_local_id, max_local);
    next_block_id = std::max(next_block_id, max_block);
    next_expr_id = std::max(next_expr_id, max_expr);
  }

  Function* find_function(FunctionId id) {
    auto it = std::find_if(functions.begin(), functions.end(),
                           [&](const Function& fn) { return fn.id.value == id.value; });
    return it == functions.end() ? nullptr : &(*it);
  }

  const Function* find_function(FunctionId id) const {
    auto it = std::find_if(functions.begin(), functions.end(),
                           [&](const Function& fn) { return fn.id.value == id.value; });
    return it == functions.end() ? nullptr : &(*it);
  }

  GlobalVar* find_global(GlobalId id) {
    auto it = std::find_if(globals.begin(), globals.end(),
                           [&](const GlobalVar& gv) { return gv.id.value == id.value; });
    return it == globals.end() ? nullptr : &(*it);
  }

  const GlobalVar* find_global(GlobalId id) const {
    auto it = std::find_if(globals.begin(), globals.end(),
                           [&](const GlobalVar& gv) { return gv.id.value == id.value; });
    return it == globals.end() ? nullptr : &(*it);
  }

  Expr* find_expr(ExprId id) {
    auto it = std::find_if(expr_pool.begin(), expr_pool.end(),
                           [&](const Expr& e) { return e.id.value == id.value; });
    return it == expr_pool.end() ? nullptr : &(*it);
  }

  const Expr* find_expr(ExprId id) const {
    auto it = std::find_if(expr_pool.begin(), expr_pool.end(),
                           [&](const Expr& e) { return e.id.value == id.value; });
    return it == expr_pool.end() ? nullptr : &(*it);
  }

  ProgramSummary summary() const {
    ProgramSummary out{};
    out.functions = functions.size();
    out.globals = globals.size();
    out.expressions = expr_pool.size();
    for (const auto& fn : functions) {
      out.blocks += fn.blocks.size();
      for (const auto& bb : fn.blocks) {
        out.statements += bb.stmts.size();
      }
    }
    return out;
  }
};

}  // namespace phase2::hir

enum class DagValueType : uint8_t {
  I1,
  I8,
  I16,
  I32,
  I64,
  F32,
  F64,
  Ptr,
  Void,
};

struct DagNodeId {
  uint32_t value = 0;

  static constexpr uint32_t kInvalid = std::numeric_limits<uint32_t>::max();

  [[nodiscard]] constexpr bool valid() const { return value != kInvalid; }
  [[nodiscard]] static constexpr DagNodeId invalid() { return DagNodeId{kInvalid}; }
};

struct DagBlockId {
  uint32_t value = 0;

  static constexpr uint32_t kInvalid = std::numeric_limits<uint32_t>::max();

  [[nodiscard]] constexpr bool valid() const { return value != kInvalid; }
  [[nodiscard]] static constexpr DagBlockId invalid() { return DagBlockId{kInvalid}; }
};

enum class DagOp : uint16_t {
  Entry,
  Param,
  Constant,
  GlobalAddr,
  FrameAddr,
  Load,
  Store,
  Add,
  Sub,
  Mul,
  SDiv,
  UDiv,
  FDiv,
  And,
  Or,
  Xor,
  Shl,
  LShr,
  AShr,
  ICmp,
  FCmp,
  Select,
  Call,
  Br,
  CondBr,
  Ret,
  PhiLike,
};

struct DagUse {
  DagNodeId node{};
  uint16_t value_index = 0;
};

struct DagNode {
  DagNodeId id{};
  DagOp op = DagOp::Entry;
  DagValueType type = DagValueType::Void;
  std::vector<DagUse> inputs;
  std::optional<long long> imm_i64;
  std::optional<double> imm_f64;
  std::optional<SymbolName> symbol;
  SourceSpan span{};
};

struct DagBlock {
  DagBlockId id{};
  std::vector<DagNodeId> nodes;
  std::vector<DagBlockId> succs;
};

namespace phase4::dag {

struct DagSummary {
  size_t functions = 0;
  size_t blocks = 0;
  size_t nodes = 0;
};

struct FunctionDag {
  FunctionId source_function{};
  std::vector<DagNode> nodes;
  std::vector<DagBlock> blocks;
  DagBlockId entry{};

  DagSummary summary() const {
    DagSummary out{};
    out.functions = 1;
    out.blocks = blocks.size();
    out.nodes = nodes.size();
    return out;
  }
};

struct ModuleDag {
  std::vector<FunctionDag> functions;
  ProgramSummary source_summary{};

  DagSummary summary() const {
    DagSummary out{};
    out.functions = functions.size();
    for (const auto& fn : functions) {
      out.blocks += fn.blocks.size();
      out.nodes += fn.nodes.size();
    }
    return out;
  }
};

}  // namespace phase4::dag

}  // namespace tinyc2ll::frontend_cxx::sema_ir
