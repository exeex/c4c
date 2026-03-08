#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
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

struct Param {
  SymbolName name;
  QualType type;
  SourceSpan span{};
};

struct LocalId {
  uint32_t value = 0;
};

struct BlockId {
  uint32_t value = 0;
};

struct ExprId {
  uint32_t value = 0;
};

struct GlobalId {
  uint32_t value = 0;
};

struct FunctionId {
  uint32_t value = 0;
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
    IndexExpr,
    MemberExpr,
    TernaryExpr,
    SizeofExpr,
    SizeofTypeExpr>;

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
  StorageClass storage = StorageClass::Auto;
  std::optional<ExprId> init;
  SourceSpan span{};
};

struct LabelRef {
  SymbolName user_name;
  BlockId resolved_block{};
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
};

struct GotoStmt {
  LabelRef target{};
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
struct BreakStmt {};
struct ContinueStmt {};

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
    LabelStmt,
    CaseStmt,
    CaseRangeStmt,
    DefaultStmt,
    BreakStmt,
    ContinueStmt>;

struct Stmt {
  StmtPayload payload{};
  SourceSpan span{};
};

struct Block {
  BlockId id{};
  std::vector<Stmt> stmts;
  bool has_explicit_terminator = false;
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
};

namespace phase2::hir {

struct Module {
  std::vector<Function> functions;
  std::vector<GlobalVar> globals;
  std::vector<Expr> expr_pool;

  std::unordered_map<SymbolName, FunctionId> fn_index;
  std::unordered_map<SymbolName, GlobalId> global_index;

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
};

struct DagBlockId {
  uint32_t value = 0;
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

struct FunctionDag {
  FunctionId source_function{};
  std::vector<DagNode> nodes;
  std::vector<DagBlock> blocks;
  DagBlockId entry{};
};

struct ModuleDag {
  std::vector<FunctionDag> functions;
  ProgramSummary source_summary{};
};

}  // namespace phase4::dag

}  // namespace tinyc2ll::frontend_cxx::sema_ir
