#pragma once

// Public HIR IR contract.
//
// Frontend, sema, codegen, dump tooling, and HIR-local transforms share these
// data structures. Keep implementation-only lowering helpers in impl/* indexes
// instead of adding them here.

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

// ── HIR pipeline stage ──────────────────────────────────────────────────────
//
// The HIR pipeline processes modules through three explicit stages:
//   1. Initial    — AST lowered to initial HIR (ast_to_hir)
//   2. Normalized — compile-time work reduced to fixpoint (compile_time_engine)
//   3. Materialized — emission decisions finalized (materialize_ready_functions)
enum class HirPipelineStage {
  Initial,       // after build_initial_hir: mixed compile-time/runtime HIR
  Normalized,    // after run_compile_time_engine: compile-time work resolved
  Materialized,  // after materialize_ready_functions: emission decisions final
};

#include "ast.hpp"
#include "source_profile.hpp"
#include "../../shared/id_hash.hpp"
#include "../../shared/text_id_table.hpp"
#include "../../target_profile.hpp"

namespace c4c::sema {
struct CanonicalType;
}  // namespace c4c::sema

namespace c4c::hir {

// This header is the migration IR contract for docs/sema_ir_split_plan.md.
//
// Phase mapping:
// - Phase 2: hir::* (typed frontend IR, AST lowering target)
// - Phase 3: backend-facing hooks embedded in HIR (attrs/debug/meta)
// - Phase 4: dag::* (lowering skeleton + inspectable DAG)
// - Phase 5+: stable IDs, summaries, and debug dumps for parity tooling

using SymbolName = std::string;

template <typename Id, typename T>
class DenseIdMap {
 public:
  [[nodiscard]] bool contains(Id id) const {
    return id.valid() && id.index() < values_.size() && values_[id.index()].has_value();
  }

  [[nodiscard]] T& at(Id id) {
    assert(contains(id) && "DenseIdMap access requires an assigned dense ID");
    return *values_[id.index()];
  }

  [[nodiscard]] const T& at(Id id) const {
    assert(contains(id) && "DenseIdMap access requires an assigned dense ID");
    return *values_[id.index()];
  }

  void insert(Id id, T value) {
    assert(id.valid() && "DenseIdMap insertion requires a valid ID");
    ensure_slot(id);
    values_[id.index()] = std::move(value);
  }

  [[nodiscard]] size_t size() const { return values_.size(); }

 private:
  void ensure_slot(Id id) {
    if (values_.size() <= id.index()) {
      values_.resize(id.index() + 1);
    }
  }

  std::vector<std::optional<T>> values_;
};

template <typename Id, typename T>
class OptionalDenseIdMap {
 public:
  [[nodiscard]] bool contains(Id id) const {
    return id.valid() && id.index() < values_.size() && values_[id.index()].has_value();
  }

  [[nodiscard]] T* find(Id id) {
    if (!contains(id)) return nullptr;
    return &*values_[id.index()];
  }

  [[nodiscard]] const T* find(Id id) const {
    if (!contains(id)) return nullptr;
    return &*values_[id.index()];
  }

  void insert(Id id, T value) {
    assert(id.valid() && "OptionalDenseIdMap insertion requires a valid ID");
    ensure_slot(id);
    values_[id.index()] = std::move(value);
  }

  template <typename Factory>
  [[nodiscard]] T& get_or_create(Id id, Factory&& factory) {
    assert(id.valid() && "OptionalDenseIdMap insertion requires a valid ID");
    ensure_slot(id);
    auto& slot = values_[id.index()];
    if (!slot) {
      slot.emplace(std::forward<Factory>(factory)());
    }
    return *slot;
  }

  [[nodiscard]] size_t size() const { return values_.size(); }

 private:
  void ensure_slot(Id id) {
    if (values_.size() <= id.index()) {
      values_.resize(id.index() + 1);
    }
  }

  std::vector<std::optional<T>> values_;
};

/// Structured namespace qualifier preserved from the AST.
/// Captures the qualifier path (e.g., ["math", "detail"] for math::detail::foo)
/// and whether the reference was global-qualified (::foo).
struct NamespaceQualifier {
  std::vector<std::string> segments;  // qualifier path (empty = unqualified)
  std::vector<TextId> segment_text_ids;  // TU-scoped text identity for qualifier segments
  bool is_global_qualified = false;   // true for ::name or ::ns::name
  int context_id = -1;               // owning namespace context id (-1 = global/unknown)

  bool empty() const { return segments.empty() && !is_global_qualified; }
};

enum class ModuleDeclKind : uint8_t {
  Function,
  Global,
};

struct ModuleDeclLookupKey {
  ModuleDeclKind kind = ModuleDeclKind::Function;
  int namespace_context_id = -1;
  bool is_global_qualified = false;
  std::vector<TextId> qualifier_segment_text_ids;
  TextId declaration_text_id = kInvalidText;

  [[nodiscard]] bool operator==(const ModuleDeclLookupKey& other) const {
    return kind == other.kind &&
           namespace_context_id == other.namespace_context_id &&
           is_global_qualified == other.is_global_qualified &&
           qualifier_segment_text_ids == other.qualifier_segment_text_ids &&
           declaration_text_id == other.declaration_text_id;
  }

  [[nodiscard]] bool operator!=(const ModuleDeclLookupKey& other) const {
    return !(*this == other);
  }
};

struct ModuleDeclLookupKeyHash {
  [[nodiscard]] size_t operator()(const ModuleDeclLookupKey& key) const noexcept {
    const size_t qualifier_hash = hash_text_id_sequence(
        key.qualifier_segment_text_ids.data(), key.qualifier_segment_text_ids.size());
    return static_cast<size_t>(hash_id_words(
        kIdHashSeed, static_cast<uint32_t>(key.kind),
        static_cast<uint32_t>(key.namespace_context_id),
        static_cast<uint32_t>(key.is_global_qualified), key.declaration_text_id,
        static_cast<uint64_t>(qualifier_hash)));
  }
};

[[nodiscard]] inline ModuleDeclLookupKey make_module_decl_lookup_key(
    ModuleDeclKind kind, const NamespaceQualifier& ns_qual, TextId declaration_text_id) {
  ModuleDeclLookupKey key;
  key.kind = kind;
  key.namespace_context_id = ns_qual.context_id;
  key.is_global_qualified = ns_qual.is_global_qualified;
  key.qualifier_segment_text_ids = ns_qual.segment_text_ids;
  key.declaration_text_id = declaration_text_id;
  return key;
}

/// Non-type template parameter value bindings (forward decl for TemplateCallInfo).
using NttpBindings = std::unordered_map<std::string, long long>;

struct SourceLoc {
  int line = 0;
};

struct SourceSpan {
  SourceLoc begin{};
  SourceLoc end{};
};

enum class Visibility : uint8_t {
  Default,
  Hidden,
  Protected,
};

struct Linkage {
  bool is_static = false;
  bool is_extern = false;
  bool is_inline = false;
  bool is_weak = false;
  Visibility visibility = Visibility::Default;
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
  int align_bytes = 0;       // from __attribute__((aligned(N)))
};

struct FnPtrSig {
  QualType return_type{};
  std::vector<QualType> params;
  bool variadic = false;
  bool unspecified_params = false;
  /// Canonical type for this function pointer (Pointer→Function).
  /// Populated during HIR lowering from sema's canonical type data.
  /// Codegen consumes this via sig_return_type() / sig_param_type() helpers.
  std::shared_ptr<sema::CanonicalType> canonical_sig;
};

struct Param {
  SymbolName name;
  TextId name_text_id = kInvalidText;
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

struct ModuleDeclLookupParityMismatch {
  ModuleDeclKind kind = ModuleDeclKind::Function;
  SymbolName name;
  uint32_t structured_id = std::numeric_limits<uint32_t>::max();
  uint32_t legacy_id = std::numeric_limits<uint32_t>::max();
};

struct HirStructDefOwnerParityMismatch {
  std::string owner_key_label;
  SymbolName rendered_tag;
  SymbolName structured_rendered_tag;
};

enum class ModuleDeclLookupAuthority : uint8_t {
  Structured,
  LegacyRendered,
  ConcreteGlobalId,
  LinkNameId,
};

struct ModuleDeclLookupHit {
  ModuleDeclKind kind = ModuleDeclKind::Function;
  ModuleDeclLookupAuthority authority = ModuleDeclLookupAuthority::Structured;
  SymbolName name;
  uint32_t resolved_id = std::numeric_limits<uint32_t>::max();
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
  LinkNameId fn_link_name_id = kInvalidLinkName;
};

struct CharLiteral {
  long long value = 0;
};

struct DeclRef {
  SymbolName name;
  TextId name_text_id = kInvalidText;
  LinkNameId link_name_id = kInvalidLinkName;
  std::optional<LocalId> local;
  std::optional<uint32_t> param_index;
  std::optional<GlobalId> global;
  NamespaceQualifier ns_qual;  // structured qualifier from source (empty for locals/params)
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
  /// Phase C: fn_ptr signature when cast target is a callable type.
  std::optional<FnPtrSig> fn_ptr_sig;
};

/// Metadata preserved for template function application calls.
struct TemplateCallInfo {
  std::string source_template;       // original template name (e.g., "add")
  TextId source_template_text_id = kInvalidText;  // stable template name in module.link_name_texts
  std::vector<TypeSpec> template_args; // resolved concrete template arguments
  NttpBindings nttp_args;             // resolved NTTP values (param name → value)
};

struct CallExpr {
  ExprId callee{};
  std::vector<ExprId> args;
  BuiltinId builtin_id = BuiltinId::Unknown;
  /// Non-null for calls that originated from a template application (e.g., add<int>).
  std::optional<TemplateCallInfo> template_info;
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
  TextId field_text_id = kInvalidText;
  SymbolName resolved_owner_tag;
  MemberSymbolId member_symbol_id = kInvalidMemberSymbol;
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

/// A consteval call that has not yet been evaluated.
/// Initial HIR construction records these nodes and leaves evaluation to the
/// compile-time engine. Some are present from the start; others are unlocked
/// later by deferred template instantiation.
struct PendingConstevalExpr {
  SymbolName fn_name;                                            // consteval function to call
  std::vector<long long> const_args;                             // evaluated constant argument values
  std::unordered_map<std::string, TypeSpec> tpl_bindings;        // template param → concrete type
  std::unordered_map<std::string, long long> nttp_bindings;      // NTTP param → constant value
  SourceSpan call_span{};
  bool unlocked_by_deferred_instantiation = false;
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
    LabelAddrExpr,
    PendingConstevalExpr>;

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

struct InlineAsmStmt {
  std::string asm_template;
  std::string constraints;
  std::optional<ExprId> output;
  bool output_is_readwrite = false;
  QualType output_type{};
  std::vector<ExprId> inputs;
  bool has_side_effects = true;
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
    InlineAsmStmt,
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

/// A stable specialization key for template instantiation identity.
/// Encodes template name + canonicalized argument types in a deterministic,
/// serializable format suitable for cross-TU dedup and future JIT caching.
///
/// Format: "template_name<param1=type1,param2=type2,...>"
/// Parameters are sorted alphabetically for determinism.
struct SpecializationKey {
  std::string canonical;  // serialized canonical form

  bool operator==(const SpecializationKey& other) const { return canonical == other.canonical; }
  bool operator!=(const SpecializationKey& other) const { return canonical != other.canonical; }
  bool operator<(const SpecializationKey& other) const { return canonical < other.canonical; }
  bool empty() const { return canonical.empty(); }
};

/// Hash support for SpecializationKey — enables use in unordered containers.
struct SpecializationKeyHash {
  std::size_t operator()(const SpecializationKey& k) const noexcept {
    return std::hash<std::string>{}(k.canonical);
  }
};

struct Function {
  FunctionId id{};
  SymbolName name;
  TextId name_text_id = kInvalidText;
  LinkNameId link_name_id = kInvalidLinkName;
  ExecutionDomain execution_domain = ExecutionDomain::Host;
  NamespaceQualifier ns_qual;  // owning namespace context from AST
  QualType return_type{};
  std::vector<Param> params;
  FnAttr attrs{};
  Linkage linkage{};
  std::vector<Block> blocks;
  BlockId entry{};
  SourceSpan span{};
  /// Phase C: fn_ptr signature of the return type when it is callable.
  std::optional<FnPtrSig> ret_fn_ptr_sig;
  /// Phase 6: materialization flag.  A function is materialized when it should
  /// be emitted as a concrete LLVM function.  Non-materialized functions remain
  /// in the module for compile-time analysis but are not emitted.
  bool materialized = true;
  /// True when this function exists only for compile-time evaluation (consteval).
  /// Consteval-only functions are lowered to HIR for analysis/debugging but are
  /// not materialized for code emission.
  bool consteval_only = false;
  /// When this function is a template instantiation, records the source template
  /// name (e.g. "add" for add<int>).  Empty for non-template functions.
  std::string template_origin;
  /// Stable specialization key for cross-TU dedup and future JIT caching.
  /// Only set for template instantiations.
  SpecializationKey spec_key;

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
  TextId field_designator_text_id = kInvalidText;
  std::optional<long long> resolved_array_bound;
  std::variant<InitScalar, std::shared_ptr<InitList>> value;
};

struct InitList {
  std::vector<InitListItem> items;
};

using GlobalInit = std::variant<std::monostate, InitScalar, InitList>;

struct GlobalVar {
  GlobalId id{};
  SymbolName name;
  TextId name_text_id = kInvalidText;
  LinkNameId link_name_id = kInvalidLinkName;
  ExecutionDomain execution_domain = ExecutionDomain::Host;
  NamespaceQualifier ns_qual;  // owning namespace context from AST
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

// ── Struct/union layout metadata (populated by hir build) ────────────────────

struct HirStructField {
  SymbolName name;
  TextId field_text_id = kInvalidText;
  MemberSymbolId member_symbol_id = kInvalidMemberSymbol;
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
  std::optional<FnPtrSig> fn_ptr_sig;  // Phase C: fn_ptr signature for callable fields
};

struct HirStructDef {
  SymbolName tag;
  TextId tag_text_id = kInvalidText;
  NamespaceQualifier ns_qual;  // owning namespace context from AST
  bool is_union = false;
  int size_bytes = 0;
  int align_bytes = 1;
  int pack_align = 0;    // #pragma pack alignment (0 = default, >0 = cap field alignment to N)
  int struct_align = 0;  // __attribute__((aligned(N))) on struct (0 = default)
  std::vector<HirStructField> fields;
  std::vector<SymbolName> base_tags;
  std::vector<TextId> base_tag_text_ids;
};

/// HIR record-owner keys identify the semantic owner of a struct/union without
/// using the rendered tag spelling that codegen and dumps still consume.
enum class HirRecordOwnerKeyKind : uint8_t {
  Declaration,
  TemplateInstantiation,
};

/// Bridge payload for template records while template identity still enters HIR
/// through declaration metadata plus a serialized specialization key.
struct HirRecordOwnerTemplateIdentity {
  TextId primary_declaration_text_id = kInvalidText;
  std::string specialization_key;

  [[nodiscard]] bool operator==(const HirRecordOwnerTemplateIdentity& other) const {
    return primary_declaration_text_id == other.primary_declaration_text_id &&
           specialization_key == other.specialization_key;
  }

  [[nodiscard]] bool operator!=(const HirRecordOwnerTemplateIdentity& other) const {
    return !(*this == other);
  }

  [[nodiscard]] bool empty() const {
    return primary_declaration_text_id == kInvalidText && specialization_key.empty();
  }
};

/// Structured semantic owner key for HIR record maps.
///
/// `rendered_tag` is deliberately absent: existing rendered-name maps remain the
/// output-spelling bridge until later dual-write steps can prove parity.
struct HirRecordOwnerKey {
  HirRecordOwnerKeyKind kind = HirRecordOwnerKeyKind::Declaration;
  int namespace_context_id = -1;
  bool is_global_qualified = false;
  std::vector<TextId> qualifier_segment_text_ids;
  TextId declaration_text_id = kInvalidText;
  HirRecordOwnerTemplateIdentity template_identity;

  [[nodiscard]] bool operator==(const HirRecordOwnerKey& other) const {
    return kind == other.kind &&
           namespace_context_id == other.namespace_context_id &&
           is_global_qualified == other.is_global_qualified &&
           qualifier_segment_text_ids == other.qualifier_segment_text_ids &&
           declaration_text_id == other.declaration_text_id &&
           template_identity == other.template_identity;
  }

  [[nodiscard]] bool operator!=(const HirRecordOwnerKey& other) const {
    return !(*this == other);
  }

  [[nodiscard]] bool has_declaration_identity() const {
    return declaration_text_id != kInvalidText;
  }

  [[nodiscard]] std::string debug_label(std::string_view rendered_fallback = {}) const {
    std::string out = "record-owner(";
    out += kind == HirRecordOwnerKeyKind::TemplateInstantiation ? "template" : "decl";
    out += " ctx=" + std::to_string(namespace_context_id);
    out += is_global_qualified ? " global" : " relative";
    out += " qual=[";
    for (size_t i = 0; i < qualifier_segment_text_ids.size(); ++i) {
      if (i != 0) out += ",";
      out += std::to_string(qualifier_segment_text_ids[i]);
    }
    out += "] decl=" + std::to_string(declaration_text_id);
    if (kind == HirRecordOwnerKeyKind::TemplateInstantiation) {
      out += " primary=" + std::to_string(template_identity.primary_declaration_text_id);
      out += " spec=";
      out += template_identity.specialization_key.empty()
          ? std::string("<none>")
          : template_identity.specialization_key;
    }
    if (!rendered_fallback.empty()) {
      out += " rendered=";
      out.append(rendered_fallback.data(), rendered_fallback.size());
    }
    out += ")";
    return out;
  }
};

struct HirRecordOwnerKeyHash {
  [[nodiscard]] size_t operator()(const HirRecordOwnerKey& key) const noexcept {
    const size_t qualifier_hash = hash_text_id_sequence(
        key.qualifier_segment_text_ids.data(), key.qualifier_segment_text_ids.size());
    const size_t specialization_hash =
        std::hash<std::string>{}(key.template_identity.specialization_key);
    return static_cast<size_t>(hash_id_words(
        kIdHashSeed, static_cast<uint32_t>(key.kind),
        static_cast<uint32_t>(key.namespace_context_id),
        static_cast<uint32_t>(key.is_global_qualified), key.declaration_text_id,
        key.template_identity.primary_declaration_text_id,
        static_cast<uint64_t>(qualifier_hash), static_cast<uint64_t>(specialization_hash)));
  }
};

struct HirStructMethodLookupKey {
  HirRecordOwnerKey owner_key;
  TextId method_text_id = kInvalidText;
  bool is_const_method = false;

  [[nodiscard]] bool operator==(const HirStructMethodLookupKey& other) const {
    return owner_key == other.owner_key &&
           method_text_id == other.method_text_id &&
           is_const_method == other.is_const_method;
  }

  [[nodiscard]] bool operator!=(const HirStructMethodLookupKey& other) const {
    return !(*this == other);
  }
};

struct HirStructMethodLookupKeyHash {
  [[nodiscard]] size_t operator()(const HirStructMethodLookupKey& key) const noexcept {
    const size_t owner_hash = HirRecordOwnerKeyHash{}(key.owner_key);
    return static_cast<size_t>(hash_id_words(
        kIdHashSeed, static_cast<uint64_t>(owner_hash), key.method_text_id,
        static_cast<uint32_t>(key.is_const_method)));
  }
};

struct HirStructMemberLookupKey {
  HirRecordOwnerKey owner_key;
  TextId member_text_id = kInvalidText;

  [[nodiscard]] bool operator==(const HirStructMemberLookupKey& other) const {
    return owner_key == other.owner_key && member_text_id == other.member_text_id;
  }

  [[nodiscard]] bool operator!=(const HirStructMemberLookupKey& other) const {
    return !(*this == other);
  }
};

struct HirStructMemberLookupKeyHash {
  [[nodiscard]] size_t operator()(const HirStructMemberLookupKey& key) const noexcept {
    const size_t owner_hash = HirRecordOwnerKeyHash{}(key.owner_key);
    return static_cast<size_t>(hash_id_words(
        kIdHashSeed, static_cast<uint64_t>(owner_hash), key.member_text_id));
  }
};

[[nodiscard]] inline HirRecordOwnerKey make_hir_record_owner_key(
    const NamespaceQualifier& ns_qual, TextId declaration_text_id) {
  HirRecordOwnerKey key;
  key.kind = HirRecordOwnerKeyKind::Declaration;
  key.namespace_context_id = ns_qual.context_id;
  key.is_global_qualified = ns_qual.is_global_qualified;
  key.qualifier_segment_text_ids = ns_qual.segment_text_ids;
  key.declaration_text_id = declaration_text_id;
  return key;
}

[[nodiscard]] inline HirRecordOwnerKey make_hir_record_owner_key(const HirStructDef& def) {
  return make_hir_record_owner_key(def.ns_qual, def.tag_text_id);
}

[[nodiscard]] inline HirRecordOwnerKey make_hir_template_record_owner_key(
    const NamespaceQualifier& ns_qual,
    TextId declaration_text_id,
    HirRecordOwnerTemplateIdentity template_identity) {
  HirRecordOwnerKey key = make_hir_record_owner_key(ns_qual, declaration_text_id);
  key.kind = HirRecordOwnerKeyKind::TemplateInstantiation;
  key.template_identity = std::move(template_identity);
  return key;
}

[[nodiscard]] inline HirRecordOwnerKey make_hir_template_record_owner_key(
    const HirStructDef& def, HirRecordOwnerTemplateIdentity template_identity) {
  return make_hir_template_record_owner_key(
      def.ns_qual, def.tag_text_id, std::move(template_identity));
}

[[nodiscard]] inline bool hir_record_owner_key_has_complete_metadata(
    const HirRecordOwnerKey& key) {
  if (key.declaration_text_id == kInvalidText) return false;
  if (std::find(key.qualifier_segment_text_ids.begin(),
                key.qualifier_segment_text_ids.end(),
                kInvalidText) != key.qualifier_segment_text_ids.end()) {
    return false;
  }
  if (key.kind == HirRecordOwnerKeyKind::TemplateInstantiation) {
    return key.template_identity.primary_declaration_text_id != kInvalidText &&
           !key.template_identity.specialization_key.empty();
  }
  return true;
}

[[nodiscard]] inline bool hir_struct_method_lookup_key_has_complete_metadata(
    const HirStructMethodLookupKey& key) {
  return hir_record_owner_key_has_complete_metadata(key.owner_key) &&
         key.method_text_id != kInvalidText;
}

[[nodiscard]] inline bool hir_struct_member_lookup_key_has_complete_metadata(
    const HirStructMemberLookupKey& key) {
  return hir_record_owner_key_has_complete_metadata(key.owner_key) &&
         key.member_text_id != kInvalidText;
}

// ── Template function definition metadata (populated by hir build) ───────────

/// Type bindings for template parameter substitution.
using TypeBindings = std::unordered_map<std::string, TypeSpec>;

/// Canonical type string for specialization keys (deterministic, no whitespace).
inline std::string canonical_type_str(const TypeSpec& ts) {
  std::string s;
  if (ts.is_const) s += "const_";
  if (ts.is_volatile) s += "volatile_";
  switch (ts.base) {
    case TB_VOID:          s += "void"; break;
    case TB_BOOL:          s += "bool"; break;
    case TB_CHAR:          s += "char"; break;
    case TB_SCHAR:         s += "schar"; break;
    case TB_UCHAR:         s += "uchar"; break;
    case TB_SHORT:         s += "short"; break;
    case TB_USHORT:        s += "ushort"; break;
    case TB_INT:           s += "int"; break;
    case TB_UINT:          s += "uint"; break;
    case TB_LONG:          s += "long"; break;
    case TB_ULONG:         s += "ulong"; break;
    case TB_LONGLONG:      s += "llong"; break;
    case TB_ULONGLONG:     s += "ullong"; break;
    case TB_FLOAT:         s += "float"; break;
    case TB_DOUBLE:        s += "double"; break;
    case TB_LONGDOUBLE:    s += "ldouble"; break;
    case TB_INT128:        s += "i128"; break;
    case TB_UINT128:       s += "u128"; break;
    case TB_STRUCT:        s += ts.tag ? std::string("struct.") + ts.tag : "struct.?"; break;
    case TB_UNION:         s += ts.tag ? std::string("union.") + ts.tag : "union.?"; break;
    case TB_ENUM:          s += ts.tag ? std::string("enum.") + ts.tag : "enum.?"; break;
    case TB_FUNC_PTR:      s += "fnptr"; break;
    default:               s += "unknown"; break;
  }
  for (int i = 0; i < ts.ptr_level; ++i) s += "*";
  if (ts.is_lvalue_ref) s += "&";
  if (ts.is_rvalue_ref) s += "&&";
  if (ts.array_rank > 0) {
    for (int i = 0; i < ts.array_rank; ++i) {
      s += "[";
      const long long dim = (i == 0) ? ts.array_size : ts.array_dims[i];
      if (dim >= 0) s += std::to_string(dim);
      s += "]";
    }
  }
  return s;
}

/// Build a specialization key from template name, parameter order, and bindings.
/// Format: "template_name<param1=type1,param2=type2>"
/// Parameters are in declaration order for determinism.
inline SpecializationKey make_specialization_key(
    const std::string& template_name,
    const std::vector<std::string>& param_order,
    const TypeBindings& bindings) {
  std::string key = template_name + "<";
  bool first = true;
  for (const auto& param : param_order) {
    if (!first) key += ",";
    first = false;
    key += param + "=";
    auto it = bindings.find(param);
    if (it != bindings.end()) {
      key += canonical_type_str(it->second);
    } else {
      key += "?";
    }
  }
  key += ">";
  return SpecializationKey{std::move(key)};
}

/// Overload that includes non-type template parameter values.
inline SpecializationKey make_specialization_key(
    const std::string& template_name,
    const std::vector<std::string>& param_order,
    const TypeBindings& bindings,
    const NttpBindings& nttp_bindings) {
  std::string key = template_name + "<";
  bool first = true;
  for (const auto& param : param_order) {
    if (!first) key += ",";
    first = false;
    key += param + "=";
    auto nttp_it = nttp_bindings.find(param);
    if (nttp_it != nttp_bindings.end()) {
      key += std::to_string(nttp_it->second);
    } else {
      auto it = bindings.find(param);
      if (it != bindings.end()) {
        key += canonical_type_str(it->second);
      } else {
        key += "?";
      }
    }
  }
  key += ">";
  return SpecializationKey{std::move(key)};
}

/// A single template instantiation produced during lowering.
struct HirTemplateInstantiation {
  std::string mangled_name;    // e.g. "add_i" for add<int>
  LinkNameId mangled_link_name_id = kInvalidLinkName;
  TypeBindings bindings;       // template param → concrete type
  NttpBindings nttp_bindings;  // non-type template param → constant value
  SpecializationKey spec_key;  // stable identity for dedup/caching
};

// ── Consteval call metadata (populated by hir build) ─────────────────────────

/// Records a consteval call that was evaluated during lowering.
/// Preserves the call intent so a future HIR pass can re-evaluate or defer.
struct ConstevalCallInfo {
  SymbolName fn_name;                           // consteval function name
  TextId fn_name_text_id = kInvalidText;        // stable consteval function name in module.link_name_texts
  std::vector<long long> const_args;            // evaluated constant argument values
  TypeBindings template_bindings;               // template param → concrete type (empty if non-template)
  long long result_value = 0;                   // computed result
  ExprId result_expr{};                         // ExprId of the IntLiteral that replaced this call
  SourceSpan span{};
};

/// A template function definition preserved through lowering.
/// The original AST body is not stored here (it would require AST lifetime management);
/// instead, this captures the template metadata and the list of instantiations produced.
struct HirTemplateDef {
  SymbolName name;                                  // generic name (e.g. "add")
  TextId name_text_id = kInvalidText;               // stable template name in module.link_name_texts
  std::vector<std::string> template_params;         // parameter names (e.g. ["T"])
  std::vector<TextId> template_param_text_ids;      // stable template parameter spellings in module.link_name_texts
  std::vector<bool> param_is_nttp;                  // true if param is non-type (int N)
  bool is_consteval = false;                        // consteval template functions
  std::vector<HirTemplateInstantiation> instances;  // instantiations produced
  SourceSpan span{};
};

struct Module {
  SourceProfile source_profile = SourceProfile::C;
  c4c::TargetProfile target_profile{};
  std::string data_layout;
  std::shared_ptr<TextTable> link_name_texts = std::make_shared<TextTable>();
  LinkNameTable link_names{link_name_texts.get()};
  MemberSymbolTable member_symbols{link_name_texts.get()};
  std::vector<Function> functions;
  std::vector<GlobalVar> globals;
  std::vector<Expr> expr_pool;

  std::unordered_map<SymbolName, FunctionId> fn_index;
  std::unordered_map<SymbolName, GlobalId> global_index;
  std::unordered_map<ModuleDeclLookupKey, FunctionId, ModuleDeclLookupKeyHash>
      fn_structured_index;
  std::unordered_map<ModuleDeclLookupKey, GlobalId, ModuleDeclLookupKeyHash>
      global_structured_index;
  mutable std::vector<ModuleDeclLookupHit> decl_lookup_hits;
  mutable std::vector<ModuleDeclLookupParityMismatch> decl_lookup_parity_mismatches;

  // Struct/union definitions (populated by build_hir)
  std::unordered_map<SymbolName, HirStructDef> struct_defs;
  std::vector<SymbolName> struct_def_order;  // insertion order for deterministic emission
  std::unordered_map<HirRecordOwnerKey, SymbolName, HirRecordOwnerKeyHash>
      struct_def_owner_index;
  std::vector<HirRecordOwnerKey> struct_def_owner_order;
  mutable std::vector<HirStructDefOwnerParityMismatch>
      struct_def_owner_parity_mismatches;

  // Template function definitions (populated by build_hir)
  std::unordered_map<SymbolName, HirTemplateDef> template_defs;

  // Consteval call records (populated by build_hir)
  std::vector<ConstevalCallInfo> consteval_calls;

  // Pipeline stage tracking — records how far this module has progressed
  // through the HIR pipeline (initial → normalized → materialized).
  HirPipelineStage pipeline_stage = HirPipelineStage::Initial;

  // Compile-time normalization info (populated by the compile-time engine in build_hir).
  // These capture the result of the HIR-owned reduction pass that runs after
  // initial lowering, so subsequent verification calls can distinguish
  // deferred work from eagerly-lowered work.
  struct CompileTimeInfo {
    size_t deferred_instantiations = 0;  // template fns instantiated by the pass
    size_t deferred_consteval = 0;       // consteval reductions unlocked by deferred instantiation
    size_t total_iterations = 0;         // fixpoint iterations performed
    size_t templates_resolved = 0;       // total template calls resolved
    size_t consteval_reduced = 0;        // total consteval calls reduced
    bool converged = false;
    std::vector<std::string> diagnostics;
    // Materialization stats (populated by Stage 3: materialize_ready_functions).
    size_t materialized_functions = 0;      // functions marked for emission
    size_t non_materialized_functions = 0;  // compile-time-only functions
  };
  CompileTimeInfo ct_info;

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

  void index_function_decl(const Function& fn) {
    fn_index[fn.name] = fn.id;
    if (fn.name_text_id == kInvalidText) return;
    fn_structured_index[make_module_decl_lookup_key(
        ModuleDeclKind::Function, fn.ns_qual, fn.name_text_id)] = fn.id;
  }

  void index_global_decl(const GlobalVar& gv) {
    global_index[gv.name] = gv.id;
    if (gv.name_text_id == kInvalidText) return;
    global_structured_index[make_module_decl_lookup_key(
        ModuleDeclKind::Global, gv.ns_qual, gv.name_text_id)] = gv.id;
  }

  void record_struct_def_owner_parity_mismatch(
      const HirRecordOwnerKey& key,
      SymbolName rendered_tag,
      SymbolName structured_rendered_tag) const {
    const std::string owner_key_label = key.debug_label(rendered_tag);
    for (const auto& mismatch : struct_def_owner_parity_mismatches) {
      if (mismatch.owner_key_label == owner_key_label &&
          mismatch.rendered_tag == rendered_tag &&
          mismatch.structured_rendered_tag == structured_rendered_tag) {
        return;
      }
    }
    struct_def_owner_parity_mismatches.push_back(
        HirStructDefOwnerParityMismatch{
            owner_key_label, std::move(rendered_tag), std::move(structured_rendered_tag)});
  }

  void index_struct_def_owner(
      const HirRecordOwnerKey& key, std::string_view rendered_tag, bool append_order) {
    if (!hir_record_owner_key_has_complete_metadata(key) || rendered_tag.empty()) return;
    SymbolName rendered(rendered_tag);
    const auto existing = struct_def_owner_index.find(key);
    if (existing != struct_def_owner_index.end() && existing->second != rendered) {
      record_struct_def_owner_parity_mismatch(key, rendered, existing->second);
    }
    if (append_order && existing == struct_def_owner_index.end()) {
      struct_def_owner_order.push_back(key);
    }
    struct_def_owner_index[key] = std::move(rendered);
  }

  void index_struct_def_owner(const HirStructDef& def, bool append_order) {
    index_struct_def_owner(make_hir_record_owner_key(def), def.tag, append_order);
  }

  [[nodiscard]] const SymbolName* find_struct_def_tag_by_owner(
      const HirRecordOwnerKey& key) const {
    const auto it = struct_def_owner_index.find(key);
    return it == struct_def_owner_index.end() ? nullptr : &it->second;
  }

  [[nodiscard]] const HirStructDef* find_struct_def_by_owner_structured(
      const HirRecordOwnerKey& key) const {
    const SymbolName* tag = find_struct_def_tag_by_owner(key);
    if (!tag) return nullptr;
    const auto it = struct_defs.find(*tag);
    return it == struct_defs.end() ? nullptr : &it->second;
  }

  [[nodiscard]] bool struct_def_owner_matches_rendered(
      const HirRecordOwnerKey& key, std::string_view rendered_tag) const {
    const SymbolName* structured_tag = find_struct_def_tag_by_owner(key);
    if (!structured_tag) return false;
    if (*structured_tag == rendered_tag) return true;
    record_struct_def_owner_parity_mismatch(
        key, SymbolName(rendered_tag), *structured_tag);
    return false;
  }

  void attach_link_name_texts(std::shared_ptr<TextTable> texts) {
    link_name_texts = texts ? std::move(texts) : std::make_shared<TextTable>();
    link_names.attach_text_table(link_name_texts.get());
    member_symbols.attach_text_table(link_name_texts.get());
  }

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

  [[nodiscard]] FunctionId lookup_function_id(std::string_view name) const {
    const auto it = fn_index.find(std::string(name));
    return it == fn_index.end() ? FunctionId::invalid() : it->second;
  }

  [[nodiscard]] GlobalId lookup_global_id(std::string_view name) const {
    const auto it = global_index.find(std::string(name));
    return it == global_index.end() ? GlobalId::invalid() : it->second;
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

  Function* find_function(LinkNameId id) {
    if (id == kInvalidLinkName) return nullptr;
    auto it = std::find_if(functions.begin(), functions.end(),
                           [&](const Function& fn) { return fn.link_name_id == id; });
    return it == functions.end() ? nullptr : &(*it);
  }

  const Function* find_function(LinkNameId id) const {
    if (id == kInvalidLinkName) return nullptr;
    auto it = std::find_if(functions.begin(), functions.end(),
                           [&](const Function& fn) { return fn.link_name_id == id; });
    return it == functions.end() ? nullptr : &(*it);
  }

  Function* find_function_by_name_legacy(std::string_view name) {
    const FunctionId id = lookup_function_id(name);
    return id.valid() ? find_function(id) : nullptr;
  }

  const Function* find_function_by_name_legacy(std::string_view name) const {
    const FunctionId id = lookup_function_id(name);
    return id.valid() ? find_function(id) : nullptr;
  }

  [[nodiscard]] std::optional<ModuleDeclLookupKey> make_decl_ref_lookup_key(
      ModuleDeclKind kind, const DeclRef& ref) const {
    if (ref.name_text_id == kInvalidText) return std::nullopt;
    if (ref.ns_qual.segment_text_ids.size() != ref.ns_qual.segments.size()) {
      return std::nullopt;
    }
    if (std::find(ref.ns_qual.segment_text_ids.begin(),
                  ref.ns_qual.segment_text_ids.end(),
                  kInvalidText) != ref.ns_qual.segment_text_ids.end()) {
      return std::nullopt;
    }
    return make_module_decl_lookup_key(kind, ref.ns_qual, ref.name_text_id);
  }

  [[nodiscard]] std::optional<ModuleDeclLookupKey> make_function_decl_lookup_key(
      const DeclRef& ref) const {
    return make_decl_ref_lookup_key(ModuleDeclKind::Function, ref);
  }

  [[nodiscard]] std::optional<ModuleDeclLookupKey> make_global_decl_lookup_key(
      const DeclRef& ref) const {
    return make_decl_ref_lookup_key(ModuleDeclKind::Global, ref);
  }

  Function* find_function_by_decl_ref_structured(const DeclRef& ref) {
    const auto key = make_function_decl_lookup_key(ref);
    if (!key) return nullptr;
    const auto it = fn_structured_index.find(*key);
    return it == fn_structured_index.end() ? nullptr : find_function(it->second);
  }

  const Function* find_function_by_decl_ref_structured(const DeclRef& ref) const {
    const auto key = make_function_decl_lookup_key(ref);
    if (!key) return nullptr;
    const auto it = fn_structured_index.find(*key);
    return it == fn_structured_index.end() ? nullptr : find_function(it->second);
  }

  void record_function_decl_lookup_parity_mismatch(
      const DeclRef& ref, const Function& structured, const Function& legacy) const {
    record_decl_lookup_parity_mismatch(
        ModuleDeclKind::Function, ref.name, structured.id.value, legacy.id.value);
  }

  [[nodiscard]] std::optional<ModuleDeclLookupHit> classify_function_decl_lookup(
      const DeclRef& ref) const {
    if (ref.local || ref.param_index || ref.global) return std::nullopt;
    if (const Function* fn = find_function(ref.link_name_id)) {
      return ModuleDeclLookupHit{
          ModuleDeclKind::Function, ModuleDeclLookupAuthority::LinkNameId, ref.name, fn->id.value};
    }
    const Function* structured = find_function_by_decl_ref_structured(ref);
    const Function* legacy = find_function_by_name_legacy(ref.name);
    if (structured && legacy && structured->id.value != legacy->id.value) {
      return ModuleDeclLookupHit{
          ModuleDeclKind::Function, ModuleDeclLookupAuthority::Structured, ref.name,
          structured->id.value};
    }
    if (structured) {
      return ModuleDeclLookupHit{
          ModuleDeclKind::Function, ModuleDeclLookupAuthority::Structured, ref.name,
          structured->id.value};
    }
    if (legacy) {
      return ModuleDeclLookupHit{
          ModuleDeclKind::Function, ModuleDeclLookupAuthority::LegacyRendered, ref.name,
          legacy->id.value};
    }
    return std::nullopt;
  }

  const Function* resolve_function_decl(const DeclRef& ref) const {
    const auto hit = classify_function_decl_lookup(ref);
    if (!hit) return nullptr;
    record_decl_lookup_hit(*hit);
    if (hit->authority == ModuleDeclLookupAuthority::LinkNameId) {
      const Function* authoritative = find_function(FunctionId{hit->resolved_id});
      const Function* legacy = find_function_by_name_legacy(ref.name);
      if (authoritative && legacy && authoritative->id.value != legacy->id.value) {
        record_function_decl_lookup_parity_mismatch(ref, *authoritative, *legacy);
      }
      return authoritative;
    }
    const Function* structured = find_function_by_decl_ref_structured(ref);
    const Function* legacy = find_function_by_name_legacy(ref.name);
    if (structured && legacy && structured->id.value != legacy->id.value) {
      record_function_decl_lookup_parity_mismatch(ref, *structured, *legacy);
      return structured;
    }
    return structured ? structured : legacy;
  }

  const Function* resolve_direct_call_callee(const DeclRef& ref) const {
    return resolve_function_decl(ref);
  }

  const Function* resolve_operator_callee(const DeclRef& ref) const {
    return resolve_function_decl(ref);
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

  GlobalVar* find_global(LinkNameId id) {
    if (id == kInvalidLinkName) return nullptr;
    auto it = std::find_if(globals.begin(), globals.end(),
                           [&](const GlobalVar& gv) { return gv.link_name_id == id; });
    return it == globals.end() ? nullptr : &(*it);
  }

  const GlobalVar* find_global(LinkNameId id) const {
    if (id == kInvalidLinkName) return nullptr;
    auto it = std::find_if(globals.begin(), globals.end(),
                           [&](const GlobalVar& gv) { return gv.link_name_id == id; });
    return it == globals.end() ? nullptr : &(*it);
  }

  GlobalVar* find_global_by_name_legacy(std::string_view name) {
    const GlobalId id = lookup_global_id(name);
    return id.valid() ? find_global(id) : nullptr;
  }

  const GlobalVar* find_global_by_name_legacy(std::string_view name) const {
    const GlobalId id = lookup_global_id(name);
    return id.valid() ? find_global(id) : nullptr;
  }

  GlobalVar* find_global_by_decl_ref_structured(const DeclRef& ref) {
    const auto key = make_global_decl_lookup_key(ref);
    if (!key) return nullptr;
    const auto it = global_structured_index.find(*key);
    return it == global_structured_index.end() ? nullptr : find_global(it->second);
  }

  const GlobalVar* find_global_by_decl_ref_structured(const DeclRef& ref) const {
    const auto key = make_global_decl_lookup_key(ref);
    if (!key) return nullptr;
    const auto it = global_structured_index.find(*key);
    return it == global_structured_index.end() ? nullptr : find_global(it->second);
  }

  void record_global_decl_lookup_parity_mismatch(
      const DeclRef& ref, const GlobalVar& structured, const GlobalVar& legacy) const {
    record_decl_lookup_parity_mismatch(
        ModuleDeclKind::Global, ref.name, structured.id.value, legacy.id.value);
  }

  [[nodiscard]] std::optional<ModuleDeclLookupHit> classify_global_decl_lookup(
      const DeclRef& ref) const {
    if (ref.local || ref.param_index) return std::nullopt;
    if (ref.global) {
      if (const GlobalVar* gv = find_global(*ref.global)) {
        return ModuleDeclLookupHit{
            ModuleDeclKind::Global, ModuleDeclLookupAuthority::ConcreteGlobalId, ref.name,
            gv->id.value};
      }
    }
    if (const GlobalVar* gv = find_global(ref.link_name_id)) {
      return ModuleDeclLookupHit{
          ModuleDeclKind::Global, ModuleDeclLookupAuthority::LinkNameId, ref.name, gv->id.value};
    }
    const GlobalVar* structured = find_global_by_decl_ref_structured(ref);
    const GlobalVar* legacy = find_global_by_name_legacy(ref.name);
    if (structured && legacy && structured->id.value != legacy->id.value) {
      return ModuleDeclLookupHit{
          ModuleDeclKind::Global, ModuleDeclLookupAuthority::Structured, ref.name,
          structured->id.value};
    }
    if (structured) {
      return ModuleDeclLookupHit{
          ModuleDeclKind::Global, ModuleDeclLookupAuthority::Structured, ref.name,
          structured->id.value};
    }
    if (legacy) {
      return ModuleDeclLookupHit{
          ModuleDeclKind::Global, ModuleDeclLookupAuthority::LegacyRendered, ref.name,
          legacy->id.value};
    }
    return std::nullopt;
  }

  const GlobalVar* resolve_global_decl(const DeclRef& ref) const {
    const auto hit = classify_global_decl_lookup(ref);
    if (!hit) return nullptr;
    record_decl_lookup_hit(*hit);
    if (hit->authority == ModuleDeclLookupAuthority::ConcreteGlobalId ||
        hit->authority == ModuleDeclLookupAuthority::LinkNameId) {
      const GlobalVar* authoritative = find_global(GlobalId{hit->resolved_id});
      const GlobalVar* legacy = find_global_by_name_legacy(ref.name);
      if (authoritative && legacy && authoritative->id.value != legacy->id.value) {
        record_global_decl_lookup_parity_mismatch(ref, *authoritative, *legacy);
      }
      return authoritative;
    }
    const GlobalVar* structured = find_global_by_decl_ref_structured(ref);
    const GlobalVar* legacy = find_global_by_name_legacy(ref.name);
    if (structured && legacy && structured->id.value != legacy->id.value) {
      record_global_decl_lookup_parity_mismatch(ref, *structured, *legacy);
      return structured;
    }
    return structured ? structured : legacy;
  }

  void record_decl_lookup_hit(const ModuleDeclLookupHit& hit) const {
    for (const auto& existing : decl_lookup_hits) {
      if (existing.kind == hit.kind && existing.authority == hit.authority &&
          existing.name == hit.name && existing.resolved_id == hit.resolved_id) {
        return;
      }
    }
    decl_lookup_hits.push_back(hit);
  }

  void record_decl_lookup_parity_mismatch(
      ModuleDeclKind kind, SymbolName name, uint32_t structured_id, uint32_t legacy_id) const {
    for (const auto& mismatch : decl_lookup_parity_mismatches) {
      if (mismatch.kind == kind && mismatch.name == name &&
          mismatch.structured_id == structured_id && mismatch.legacy_id == legacy_id) {
        return;
      }
    }
    decl_lookup_parity_mismatches.push_back(
        ModuleDeclLookupParityMismatch{kind, std::move(name), structured_id, legacy_id});
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

namespace dag {

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

}  // namespace dag

std::string encode_template_type_arg_ref_hir(const TypeSpec& ts);

}  // namespace c4c::hir
