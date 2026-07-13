# Bootstrap BIR Schema and API Checkpoint

Status: repaired implementation checkpoint; pending independent Step 2 re-review  
Authority: `03_pass_ready_bir_contract.md` and
`05_target_schema_and_api_blueprint.md`  
Scope: the smallest real BIR core needed to reconnect the retained LIR import
spine and CFG, without reviving legacy BIR, prealloc, preparation, or target
state.

This checkpoint intentionally chooses concrete bootstrap semantics where the
715 research permitted multiple implementations. Step 3 and Step 4 must follow
these choices; changing them requires another review checkpoint.

## 1. Initial files and dependency direction

The initial implementation is deliberately smaller than the complete 715 tree:

```text
src/backend/bir/
  bir.hpp                 public facade; includes only core/view.hpp and builder.hpp
  core/
    result.hpp            C++17 Result<T, E>, success/error factories
    ids.hpp               epochs, slots, generations, stable IDs, hash support
    storage.hpp           SlotMap and IdOrder
    type.hpp              bounded semantic Type
    ir.hpp                payloads and private owning ModuleData
    view.hpp              ModuleView, FunctionView, read-only traversal
    builder.hpp           ModuleBuilder, FunctionBuilder, RawBir declarations
    builder.cpp           checked construction and publication
  verify/
    verifier.hpp          foundation profile and structured errors
    verifier.cpp          foundation rules
```

Dependency direction is strictly:

```text
result,ids <- storage
ids,type <- ir
ir <- view
ir,view,verifier <- builder
core/view,core/builder <- bir.hpp
bir.hpp <- lir_to_bir adapter
```

`result.hpp`, `ids.hpp`, `storage.hpp`, `type.hpp`, `ir.hpp`, and `view.hpp` may include only
standard-library headers and earlier core headers. `verify/` may include core.
Only `lir_to_bir/` may include LIR. Nothing in this initial tree may include or
forward declarations from `src/backend/legacy`, prealloc, MIR, a target backend,
compatibility, preparation, passes, or analysis. No proposed source or include
path is under `src/backend/legacy`.

`bir.hpp` exposes IDs, bounded semantic enums/specs, views, builders, `RawBir`,
and verification diagnostics. It does not expose `ModuleData`, `FunctionData`,
`SlotMap`, mutable order containers, or mutable node references.

## 2. Stable identities and validity

The bootstrap uses fixed-width, nonzero components:

```cpp
using ModuleEpoch = std::uint64_t;
using SlotIndex = std::uint32_t;
using Generation = std::uint32_t;

struct FunctionId {
  ModuleEpoch epoch;
  SlotIndex slot;
  Generation generation;
};
struct BlockId {
  FunctionId owner;
  SlotIndex slot;
  Generation generation;
};
struct InstId {
  FunctionId owner;
  SlotIndex slot;
  Generation generation;
};
enum class ValueKind : std::uint8_t { Parameter, InstResult };
struct ValueId {
  FunctionId owner;
  ValueKind kind;
  SlotIndex slot;
  Generation generation;
};
```

Zero is invalid for `ModuleEpoch` and `Generation`. A default-constructed ID is
invalid. Slot zero is valid; validity never depends on slot position or order.
The process-local epoch allocator returns monotonically increasing nonzero
epochs. Exhausting `uint64_t` is a hard publication failure; it never wraps.

Equality compares every field, recursively including the owner. Ordering is not
semantic and no relational operator is provided. `std::hash` combines every
field used by equality, including `ValueKind`; hashes are lookup aids, never
identity. IDs are trivially copyable values, not implicit indices.

Resolution requires all of:

1. the requested module epoch equals `ModuleData::epoch`;
2. a nested ID's full `FunctionId owner` resolves in that module;
3. the slot is in range and live;
4. the stored generation equals the ID generation;
5. for `ValueId`, stored `ValueDef::kind` equals the ID kind.

Any mismatch returns `ResolveError::{WrongEpoch, WrongOwner, OutOfRange,
Tombstone, StaleGeneration, WrongKind}`. It must not assert, alias another
object, consult a name, or fall back to a vector position. Passing a `BlockId`,
`InstId`, or `ValueId` owned by a different function to a builder returns
`BuildError::ForeignOwner` before mutation.

## 3. Generation-checked storage and semantic order

All fallible bootstrap APIs use the repo-local, C++17-only carrier declared in
`src/backend/bir/core/result.hpp`:

```cpp
template<class T, class E>
class Result {
 public:
  static Result success(T value);
  static Result failure(E error);
  bool has_value() const noexcept;
  explicit operator bool() const noexcept;
  T& value() &;
  const T& value() const&;
  T&& value() &&;
  E& error() &;
  const E& error() const&;
 private:
  std::variant<T, E> storage_;
};

template<class E>
class Result<void, E> {
 public:
  static Result success();
  static Result failure(E error);
  bool has_value() const noexcept;
  explicit operator bool() const noexcept;
  E& error() &;
  const E& error() const&;
 private:
  std::variant<std::monostate, E> storage_;
};
```

`Result` owns exactly one value or error, has no default constructor, performs
no implicit value/error conversion, and never throws for a reported domain
failure. Calling the inactive accessor is a programmer-contract assertion.
`Result<T, E>` requires distinct `T` and `E`; neither may be a reference or
`void` (the specialization owns the void-success case). No API uses
`std::expected` or a reference-valued result. Internal checked borrowing uses
`Result<std::reference_wrapper<const T>, E>` and
`Result<std::reference_wrapper<T>, E>`; public views return IDs, value copies,
or dedicated read-only view values instead of exposing those wrappers.

The bootstrap storage contract is:

```cpp
template<class T>
struct Slot {
  Generation generation = 1;
  std::optional<T> value;       // nullopt is a tombstone/free slot
};

template<class T, class Id, class Owner>
class SlotMap {
 public:
  Result<Id, StorageError> emplace(const Owner&, T);
  Result<std::reference_wrapper<const T>, ResolveError>
      get(const Owner&, Id) const;
  Result<std::reference_wrapper<T>, ResolveError>
      get_mut(const Owner&, Id); // builder/verifier-private
  Result<void, StorageError> erase(const Owner&, Id);
  bool contains(const Owner&, Id) const;
 private:
  std::vector<Slot<T>> slots_;
  std::vector<SlotIndex> free_;
};
```

Erase destroys `value`, then increments the generation before adding the slot
to `free_`. If increment would wrap to zero, that slot is permanently retired
and never re-enters `free_`. Allocation failure caused by slot-index exhaustion
is reported; no slot or generation wraps. This no-wrap policy is sufficient for
bootstrap and prevents a stale ID ever resolving to a reused entity.

Storage owns lifetime only. Traversal order is separate:

```cpp
template<class Id>
class IdOrder {
 public:
  const std::vector<Id>& ids() const noexcept;
 private:
  Result<void, OrderError> append(Id);
  Result<void, OrderError> erase(Id);
  std::vector<Id> ids_;
};
```

Only builders (and later editors) mutate `IdOrder`. Duplicate insertion,
erase of a non-member is an error. Module function order,
function block layout order, and block instruction order each use an `IdOrder`.
Allocation/free-list order has no observable meaning. Step 3 implements
allocation, resolution, append, and read traversal; erase/reuse is implemented
and unit-provable internally but is not exposed through the public facade.

## 4. Bounded semantic payloads

The bootstrap payload supports the import spine plus CFG publication, not all
currently retained adapter families.

```cpp
enum class TypeKind : std::uint8_t {
  Void, I1, I8, I16, I32, I64, F32, F64, Pointer
};
struct Type { TypeKind kind; };

struct ParameterDef { std::uint32_t ordinal; };
struct InstResultDef { InstId instruction; std::uint16_t result_index; };

struct ValueDef {
  ValueKind kind;
  Type type;
  std::variant<ParameterDef, InstResultDef> definition;
};
```

`Type` is semantic and closed in bootstrap. Structured/aggregate/vector types,
integer widths not listed above, function types, and type interning are deferred.
The importer must reject them until a later packet extends this type algebra.
Names and original type spellings may remain adapter-local diagnostics only.

```cpp
enum class Opcode : std::uint8_t { /* no bootstrap ordinary opcode */ };

struct InstData {
  Opcode opcode;
  std::vector<ValueId> operands;
  std::vector<ValueId> results;
};

struct JumpTerm { BlockId target; };
struct CondJumpTerm { ValueId condition; BlockId true_target; BlockId false_target; };
struct ReturnTerm { std::optional<ValueId> value; };
struct UnreachableTerm {};
using Terminator = std::variant<JumpTerm, CondJumpTerm, ReturnTerm,
                                UnreachableTerm>;

struct BlockData {
  IdOrder<InstId> instruction_order;
  std::optional<Terminator> terminator; // incomplete only while building
  std::string debug_name;               // non-authoritative
};

struct FunctionSignature {
  Type return_type;
  std::vector<Type> parameter_types;
  bool is_variadic = false;
};
struct FunctionData {
  FunctionSignature signature;
  bool is_declaration = false;
  std::string link_name; // link identity at bootstrap, never local entity identity
  SlotMap<BlockData, BlockId, FunctionId> blocks;
  SlotMap<InstData, InstId, FunctionId> insts;
  SlotMap<ValueDef, ValueId, FunctionId> values;
  IdOrder<BlockId> block_order;
  std::vector<ValueId> parameters;
};

struct ModuleData {
  ModuleEpoch epoch;
  SlotMap<FunctionData, FunctionId, ModuleEpoch> functions;
  IdOrder<FunctionId> function_order;
  std::unordered_map<std::string, FunctionId> functions_by_link_name;
};
```

Concrete constants are intentionally absent from this first payload. A
conditional branch therefore accepts only a previously created local value;
constant values arrive with the scalar-family packet. Phi and block arguments
are absent from the bootstrap schema: the CFG importer must reject blocks
containing LIR phi until one named packet lands incoming-edge representation and
verification together.

The initial terminators are exactly `Jump`, `CondJump`, `Return`, and
`Unreachable`. `successors(block)` pattern-matches the terminator and returns:
one target for `Jump`, two ordered targets for `CondJump` (even if equal), and
none for `Return`/`Unreachable`. There is no successor field on `BlockData`, no
predecessor field anywhere, and no CFG side table in `ModuleData` or
`FunctionData`. Labels are resolved to `BlockId` by an adapter-private map before
calling `set_terminator`.

## 5. Construction API

The public construction surface required by the first migration packets is:

```cpp
class FunctionBuilder;
struct BuildResult { InstId instruction; std::vector<ValueId> results; };
struct UnsupportedInstSpec {};

class ModuleBuilder {
 public:
  ModuleBuilder();
  ModuleBuilder(const ModuleBuilder&) = delete;
  ModuleBuilder& operator=(const ModuleBuilder&) = delete;
  ModuleBuilder(ModuleBuilder&&) = delete;
  ModuleBuilder& operator=(ModuleBuilder&&) = delete;
  Result<FunctionId, BuildError> create_function(
      FunctionSignature, std::string link_name, bool is_declaration);
  using FunctionEdit =
      std::function<Result<void, BuildError>(FunctionBuilder&)>;
  Result<void, BuildError> with_function(FunctionId, FunctionEdit);
  Result<RawBir, PublishError> publish() &&;
};

class FunctionBuilder {
 public:
  FunctionBuilder(const FunctionBuilder&) = delete;
  FunctionBuilder& operator=(const FunctionBuilder&) = delete;
  FunctionBuilder(FunctionBuilder&&) = delete;
  FunctionBuilder& operator=(FunctionBuilder&&) = delete;
  FunctionId id() const;
  Result<ValueId, BuildError> parameter(std::uint32_t ordinal) const;
  Result<BlockId, BuildError> create_block(std::string debug_name = {});
  Result<BuildResult, BuildError> append(BlockId, UnsupportedInstSpec);
  Result<void, BuildError> set_terminator(BlockId, TerminatorSpec);
 private:
  FunctionBuilder(ModuleBuilder&, FunctionId, std::uint64_t scope_token);
  ModuleBuilder* parent_;
  FunctionId function_;
  std::uint64_t scope_token_;
  friend class ModuleBuilder;
};
```

In `builder.hpp`, `RawBir` is defined first (with `ModuleBuilder` forward
declared as its friend), followed by `BuildResult`/`UnsupportedInstSpec`, the
`FunctionBuilder` forward declaration, `ModuleBuilder`, and finally
`FunctionBuilder`. Thus `Result<RawBir, PublishError>` sees a complete success
type and the callback sees a declared capability; all by-value result/spec types
are complete before use.

`ModuleBuilder` exclusively owns `ModuleData` and has state
`Open|EditingFunction|Consumed`. It never moves or copies. `with_function`
validates the ID, changes `Open` to `EditingFunction`, increments a nonzero
scope token, constructs one stack-local `FunctionBuilder`, invokes the callback
synchronously, destroys the capability, and returns to `Open`. Nested or
concurrent `with_function`, `create_function`, or `publish` while editing fails
with `BuildError::ActiveFunctionEdit`. Every `FunctionBuilder` operation checks
the parent's state and exact scope token; an expired token returns
`BuildError::ExpiredCapability`. The capability cannot copy, move, or be
returned by value. Multiple capabilities never coexist. A callback must not
retain its reference; the object is destroyed before `with_function` returns.

`publish() &&` is callable only in `Open`, consumes `data_`, changes the builder
permanently to `Consumed` on success, and returns `AlreadyConsumed` on any later
operation. Verification failure leaves the builder `Open` with its data intact
so diagnostics can be inspected and construction repaired; it does not create
`RawBir`. Thus no mutable capability exists when ownership reaches `RawBir`.

`create_function` requires a non-empty link name. `ModuleData` maintains the
bootstrap `functions_by_link_name` index as non-semantic lookup infrastructure:
stable identity remains `FunctionId`. A new name allocates one function and its
parameters in signature order and appends it to module order. For an existing
name, an exactly matching signature/variadic contract obeys these rules:

- declaration after declaration returns the existing ID;
- definition after declaration upgrades that same `FunctionData` to a
  definition and returns the existing ID;
- declaration after definition returns the existing ID;
- definition after definition returns `BuildError::DuplicateDefinition`.

Any signature mismatch returns `BuildError::ConflictingDeclaration`; empty
names return `BuildError::EmptyLinkName`. Rejected operations do not mutate the
index, storage, or order. Full `SymbolId` storage, visibility, linkage classes,
renaming, and overload/mangling policy remain deferred.

A declaration rejects block creation. A definition may be incomplete until
publication. `create_block` only appends layout order.

`append` is present for API stability but returns
`BuildError::UnsupportedOpcode` in the foundation packet because the sole
`UnsupportedInstSpec` carries no semantics and the bounded
opcode set is empty. Each later family adds a concrete `InstSpec` variant,
descriptor checks, result allocation, and verifier rule together.

`set_terminator` accepts specs whose IDs are already resolved. It rejects a
second terminator, foreign owners, unknown targets, non-`I1` conditions, and a
return value incompatible with the signature. `Unreachable` has no operands.
Forward label references are adapter concerns: the CFG packet first creates all
blocks, builds `label -> BlockId`, then installs terminators. No unresolved
label/token enters `ModuleData`.

`lowering.hpp`, `context.cpp`, `module.cpp`, and `types.cpp` need module creation,
function declaration/definition creation, parameter lookup, block creation, and
typed safe-rejection diagnostics. `cfg.cpp` needs the adapter-private LIR label
lookup plus `create_block`, `set_terminator`, and view successors. No
current need justifies public globals, locals, calls, layout tables, ABI records,
or prepared state.

## 6. Publication and read-only views

```cpp
class RawBir {
 public:
  RawBir(RawBir&&) noexcept;
  RawBir& operator=(RawBir&&) noexcept;
  RawBir(const RawBir&) = delete;
  RawBir& operator=(const RawBir&) = delete;
  ModuleView view() const;
 private:
  explicit RawBir(std::unique_ptr<ModuleData>, RawStateToken);
  std::unique_ptr<ModuleData> data_;
  RawStateToken token_;
  friend class ModuleBuilder;
};
```

Only `ModuleBuilder::publish() &&` can construct `RawBir`, and only after the
foundation verifier succeeds. Failure returns structured diagnostics and does
not expose storage. `RawStateToken` has no public constructor.

`ModuleView` is a borrowed, read-only view whose lifetime is bounded by its
`RawBir`. It exposes `epoch()`, ordered `functions()`, `function(FunctionId)`,
and `verify()`. `FunctionView` exposes `id()`, signature/declaration/link-name,
ordered `parameters()`, ordered `blocks()`, `block(BlockId)`, `value(ValueId)`,
ordered `instructions(BlockId)`, `terminator(BlockId)`, and
`successors(BlockId)`. Resolution returns `Result<ViewValue, ResolveError>`,
never a persistent raw pointer or reference-valued result. Iteration yields IDs.
No mutable span/container/data pointer is public.

The bootstrap deliberately does not add `Handle<T>`, mutation-scoped `Ref<T>`,
`CanonicalBir`, or a canonicalization session; IDs plus publication-scoped views
are sufficient for Steps 3--5. Their absence must not be filled with raw-pointer
or positional convenience APIs.

## 7. Foundation verifier

`VerifyProfile::FoundationRaw` produces `VerificationResult` containing zero or
more `VerificationError {rule, function, entity, message}`. Publication fails
on any error. The initial rules are:

1. module epoch and every ID component are nonzero where required;
2. every storage entry resolves by exact epoch/owner/generation/kind;
3. each live function appears exactly once in function order and no tombstone
   or unknown ID appears there;
4. every function has a non-empty link name; live function link names are
   unique; `functions_by_link_name` has exactly one entry per live function and
   resolves to that same function; no stale/foreign/tombstone ID or extra index
   key exists;
5. each live block appears exactly once in its owning function block order;
6. each live instruction appears exactly once in exactly one owning block's
   instruction order; every listed instruction resolves to that function;
7. each parameter appears once in signature order, has matching ordinal/type,
   and every value definition resolves to its owning block/instruction;
8. declarations have no blocks; definitions have at least one block and every
   block has exactly one terminator at publication;
9. every terminator target and local operand has the same `FunctionId` owner;
   every target resolves; condition is `I1`; return arity/type matches signature;
10. no unresolved builder token or active function edit exists;
11. all bounded enums/variants hold known alternatives.

Def-use lists, dominance, phi predecessor equality, opcode descriptors,
canonical-stage legality, reachability policy, and analysis revision checks are
not foundation rules because their data does not yet exist. They must be added
atomically with the relevant schema packet, not guessed in Step 3.

## 8. Current import-spine and CFG mapping

AST-backed queries were attempted first. The compile database identifies the
translation units, but parsing is currently incomplete because
`src/backend/bir/lir_to_bir/lowering.hpp` includes the intentionally missing
`../lir_to_bir.hpp`. The queries still recovered the named functions below;
focused source search supplied the mutation sites without treating legacy types
as a target API.

| Current symbol/file | Proposed boundary | Bootstrap disposition |
|---|---|---|
| `BirLoweringContext`, `make_lowering_context` (`context.cpp`) | owns diagnostics and a `ModuleBuilder`; later returns `RawBir` | retain adapter-local options/notes; reject target/prepared options |
| `analyze_module` (`analysis.cpp`) | adapter preflight only | no analysis payload enters BIR; unsupported facts reject |
| `lower_module` (`module.cpp`) | `ModuleBuilder`, then `publish()` | return `Result<RawBir, ImportError>`, not `optional<bir::Module>` |
| `BirFunctionLowerer::lower`, `lower_extern_decl`, `lower_decl_function` (`module.cpp`, declarations in `lowering.hpp`) | `create_function`, parameters, `FunctionBuilder` | signatures limited to bounded scalar/pointer types |
| `lower_param_type`, `lower_signature_return_info`, `lower_integer_type` (`lowering.hpp`/`types.cpp`) | construct bounded `Type` | structured/aggregate/vector/unlisted widths reject |
| `build_bir_structured_type_spelling_context` and layout/parity helpers (`types.cpp`) | none in core | adapter diagnostic only; a request to publish it rejects |
| direct writes to `module.functions`, `function.blocks`, `block.insts` (`module.cpp`) | builder calls and explicit orders | direct container construction is removed during migration |
| `make_block_lookup` (`cfg.cpp`) | adapter `label -> BlockId` after creating all blocks | labels never become identity or published CFG authority |
| `follow_empty_branch_chain`, `follow_canonical_select_chain` (`cfg.cpp`) | adapter preflight or future canonical pass | must not rewrite foundation BIR silently; reject if required for acceptance |
| `collect_phi_lowering_plans` (`cfg.cpp`) and phi construction in `module.cpp` | future phi/block-argument packet with incoming-edge schema | phi-bearing LIR rejects in bootstrap |
| `lower_block_terminator` (`module.cpp`) | `set_terminator` with resolved IDs | only jump/cond-jump/return/unreachable accepted |
| switch-created trailing blocks (`module.cpp`) | no bootstrap API | reject switch; block insertion/order mutation arrives with its instruction family |

Every active retained file is assigned a migration boundary:

- import spine/CFG now: `lowering.hpp`, `context.cpp`, `module.cpp`, `types.cpp`,
  `cfg.cpp`;
- globals later: `globals.cpp`, `global_initializers.cpp`;
- scalar/aggregate later: `scalar.cpp`, `aggregate.cpp`;
- memory later: `memory/memory_types.hpp`, `memory/memory_helpers.hpp`,
  `memory/local_slots.cpp`, `memory/addressing.cpp`, `memory/provenance.cpp`,
  `memory/value_materialization.cpp`, `memory/local_gep.cpp`,
  `memory/intrinsics.cpp`, `memory/coordinator.cpp`;
- calls/analysis later: `calling.cpp`, `call_abi.cpp`, `analysis.cpp`.

The safe-rejection boundary is exact: globals reject until module-owned global
IDs and payloads exist; ordinary scalar instructions reject until their opcode,
operands, results, and verifier rules exist; aggregate/structured types reject;
alloca/load/store/GEP/intrinsics reject; calls reject; phi and switch reject;
adapter analysis may decide rejection but may not publish route, provenance,
layout, ABI, or cached facts. Rejection returns an `ImportError` naming family,
function, block when known, and LIR operation/type. It never emits an empty
function/module for meaningful unsupported input.

## 9. Independent implementation packets

### Step 3A — identities and containers

Add `result.hpp`, `ids.hpp`, and `storage.hpp` with the C++17 result carrier,
equality/hash, exact borrowed-resolution representation, generation
tombstones/no-wrap behavior, and separate `IdOrder`. Add no facade or import
changes. Proof: compile a focused core target or translation unit and
packet-local result/identity/storage checks.

### Step 3B — bounded ownership and views

Add `type.hpp`, `ir.hpp`, and `view.hpp`; implement module/function/block/value
ownership, explicit orders, terminator-derived successors, and read-only ID
traversal. Add `bir.hpp` as the narrow facade. Do not add builders yet. Proof:
default build reaches the missing construction seam and compile metadata remains
legacy-free.

### Step 4A — builders and move-only publication

Add `builder.hpp/.cpp`; implement non-moving module ownership, scoped
single-function capabilities, link-name merge/index policy,
function/parameter/block creation, terminators, checked owner/type rejection,
and move-only `RawBir`. `append` rejects until an opcode family lands. Proof:
focused builder construction and negative checks through the public facade.

### Step 4B — foundation verification gate

Add `verifier.hpp/.cpp`, implement the eleven foundation rules, and require success
inside `publish() &&`. Proof: malformed state is exercised only through
builder-private packet tests; the durable public proof remains the retained
LIR-to-BIR boundary when Step 5 connects it.

These packets are review/build independent. Step 3A does not require semantic
payloads; Step 3B does not expose mutation; Step 4A cannot publish unchecked
state as accepted output; Step 4B does not add semantic families.

## 10. Explicitly deferred 715 work

Deferred until a named migration need: module symbol/type/global IDs; constants;
locals; complete opcode descriptors; structured/aggregate/vector/function
types; globals; calls; memory and atomics; intrinsics and inline asm; phi/block
arguments and their incoming-edge schema; complete def-use;
handles and mutation-scoped refs; editor transactions, RAUW, split/redirect/
erase APIs; revisions and mutation summaries; CFG/dominance/provenance/effect/
call-graph analyses; canonical passes and `CanonicalBir`; compatibility capsule;
PreparedBir, ABI/frame/allocation/address plans; MIR and instruction selection;
parallel module barriers.

Deferral is not permission to use names, raw pointers, vector indices, route
records, or legacy structs in their place.

## 11. Anti-legacy acceptance checks

Before accepting any Step 3/4 packet:

- no compile-command entry or include names `src/backend/legacy`;
- no new core field/type contains `Legacy`, `Route`, `Prealloc`, `Prepared`,
  register/stack assignment, frame offset, target opcode, printer, dump cache,
  or debug-focus authority;
- no source is copied, included, wrapped, aliased, or re-exported from legacy;
- no persistent raw pointer, vector position, display name, block label, slot
  name, or instruction index acts as semantic identity;
- no successor/predecessor side table duplicates terminator authority;
- no unsupported LIR family succeeds by omission or empty publication;
- no adapter-private layout/spelling/provenance/ABI map enters `ModuleData`.

This checkpoint is complete when a Step 3 implementer can build ownership,
identity, storage, traversal, and facade semantics exactly as written, and a
Step 4 implementer can add construction/publication without inventing a legacy
compatibility surface.
