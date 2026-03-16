# Canonical Type Refactor Plan

## Goal

Prepare the semantic pipeline for future `.cpp` and `.c4` support by introducing a real canonical type system after parsing.

The immediate objective is to separate:

- parser-facing declarator shape
- sema-facing resolved type identity
- lowering-facing callable prototype information

while keeping:

- existing parser AST intact
- existing C behavior fully preserved
- existing HIR/codegen behavior working during migration

This refactor is a prerequisite for reliable:

- indirect call lowering
- function prototype propagation
- C++ mangling
- overload-ready symbol identity
- future template instantiation typing

## Constraints

- No full C++ semantic implementation is required yet
- No template instantiation engine is required yet
- Parser AST should remain usable during the migration
- Existing `TypeSpec`-based logic may coexist temporarily with canonical types
- Compatibility with existing C tests is the top priority
- The first goal is architectural correctness, not immediate total replacement

## High-Level Direction

The frontend should distinguish three type layers:

1. Declarator representation in the parser
2. Resolved canonical type in sema
3. Backend-consumable prototype/signature data derived from canonical type

Desired flow:

`source -> parser AST/TypeSpec -> sema resolution -> CanonicalType -> validation/lowering/mangling`

The main design rule is:

- `TypeSpec` is parser-oriented
- `CanonicalType` is semantic truth
- lowering and mangling should consume canonical type data, not reconstruct prototypes ad hoc

## Core Refactor Principles

### 1. Parser shape is not semantic truth

The parser should continue to produce the current declarator-oriented structures:

- `TypeSpec`
- `Node::params`
- `Node::fn_ptr_params`
- `Node::variadic`

These are useful parsing artifacts, but they are not a strong long-term semantic type model.

Examples of why:

- function types are split across multiple AST fields
- function pointer information is partially in `TypeSpec` and partially on `Node`
- arrays, pointers, and function layers are encoded in a flattened form
- future template substitution cannot be represented cleanly using only `TypeSpec`

This implies a strict responsibility boundary:

- `TypeSpec` may describe parser/declarator shape
- `TypeSpec` must not remain a long-term second semantic type system

If a piece of logic needs semantic type identity after sema, that logic should eventually stop depending on `TypeSpec`.

### 2. Sema owns canonicalization

Canonical type construction should happen after parsing, when sema has enough information to normalize type identity.

This includes:

- primitive types
- qualified types
- pointer layers
- array layers
- function types
- function pointer types
- later: member function qualifiers
- later: template parameter and template argument substitution

This means canonical type generation belongs to sema, not to the parser.

### 3. Canonical type must be recursive

The canonical type system must represent compound types structurally rather than as flattened flags.

Bad direction:

- `base + ptr_level + is_fn_ptr + n_fn_ptr_params`

Good direction:

- leaf types for primitives and named types
- recursive wrappers for pointer and array
- recursive function signatures with return type and parameter types

This is required for:

- indirect call analysis
- function pointer declarations
- prototype compatibility checks
- mangling
- template-aware type substitution later

### 4. One semantic type representation should dominate after sema

Once sema has resolved a declaration or expression type, downstream stages should prefer canonical type instead of repeatedly reconstructing meaning from AST fragments.

This does not require deleting `TypeSpec` immediately.

It does require establishing:

- canonical type as the authoritative sema result
- a stable way to retrieve canonical type from declarations and expressions
- a migration rule for deleting overlapping `TypeSpec` responsibilities once each consumer is moved

The intended end state is:

- parser owns `TypeSpec`
- sema/lowering/mangling own and consume `CanonicalType`

There should not be two equally-authoritative semantic type paths.

### 5. Prototype information should travel through canonical type

Function prototype information should not be carried through ad hoc side channels wherever possible.

In particular:

- function declarations should resolve to canonical function type
- function pointer declarations should resolve to pointer-to-function canonical type
- indirect call analysis should consume canonical function signatures
- mangling should consume canonical function signatures

This is the key reason this refactor must happen before deeper C++ work.

## Target Model

## Objective

Introduce a recursive `CanonicalType` and a sema-owned resolved type context.

## Target shape

Possible model:

```cpp
enum class CanonicalTypeKind {
  Void,
  Bool,
  Int,
  Struct,
  Enum,
  TypedefName,
  Pointer,
  Array,
  Function,
  VendorExtended,
};

struct CanonicalFunctionSig {
  CanonicalType return_type;
  std::vector<CanonicalType> params;
  bool variadic;
  bool unspecified_params;
};

struct CanonicalType {
  CanonicalTypeKind kind;
  Qualifiers quals;

  std::shared_ptr<CanonicalType> element_type;   // for pointer / array
  std::shared_ptr<CanonicalFunctionSig> fn_sig;  // for function

  std::string nominal_name;  // struct/enum/typedef/vendor name when applicable
  long long array_size;
};
```

Exact field names may vary, but the recursive structure should remain.

## Sema-resolved type context

Sema should expose a stable resolved-type result rather than forcing each consumer to recanonicalize independently.

Possible model:

```cpp
struct ResolvedTypeTable {
  std::unordered_map<const Node*, CanonicalType> node_types;
};
```

Or:

```cpp
struct AnalyzeResult {
  ValidateResult validation;
  SemaCanonicalResult canonical;
  ResolvedTypeTable resolved_types;
  std::optional<HirModule> hir_module;
};
```

The exact storage mechanism can vary:

- side table keyed by `Node*`
- sema context object
- declaration/expression annotations

The important property is:

- resolved canonical type should be computed once in sema
- later stages should read it instead of rebuilding it loosely

## Immediate Type Coverage

The first canonical type implementation should support:

- all existing C primitive types
- qualifiers already expressible in `TypeSpec`
- pointer types
- array types
- top-level function types
- function pointer types
- variadic signatures
- unspecified parameter-list signatures
- named nominal types like struct/union/enum/typedef

This is enough to support C correctness and unblock indirect-call and mangling work.

## Deferred Type Coverage

The initial canonical refactor does not need to fully model:

- member function qualifiers
- references
- rvalue references
- template-dependent types
- substitution nodes
- overload sets
- non-type template arguments beyond placeholders
- C++ method pointers

These should be reserved in the structure design, but not required for phase 1 migration.

## Why Indirect Call Is The Forcing Case

Indirect call handling is the clearest current reason to introduce canonical type as the sema truth.

Example:

```cpp
int (*fp)(int, int);
int result = fp(20, 22);
```

The call analysis and lowering need to know:

- callee is callable
- callee type is pointer-to-function
- function return type
- parameter types
- variadic or not
- unspecified-prototype or not

This information should be read directly from canonical type:

- `Pointer(Function(...))`

It should not be reconstructed from:

- `TypeSpec::is_fn_ptr`
- `Node::fn_ptr_params`
- special lowering-side heuristics

## Relationship To Mangling

Canonical type is the type-layer prerequisite for mangling, but it is not mangling itself.

The expected dependency direction is:

- sema resolves declarations and expressions into canonical types
- sema builds canonical symbols using canonical types
- mangler encodes canonical symbols into strings

This is especially important for:

- function prototypes
- function pointer parameters
- template instantiations later
- overloaded function identity later

If prototype information does not already live in canonical type, mangling will be forced to rebuild semantic meaning from parser artifacts, which is the wrong layering.

## Migration Strategy

## Objective

Move the codebase from parser-shaped type consumption to sema-shaped canonical type consumption without breaking existing C behavior.

## Recommended migration rule

Do not attempt to replace every `TypeSpec` use at once.

Instead:

- preserve parser and AST structures
- introduce canonical type as a sema product
- migrate downstream consumers incrementally

## Responsibility reduction rule

Where `TypeSpec` and `CanonicalType` overlap semantically, the overlap should be treated as temporary migration debt.

The rule is:

- if a semantic consumer has been migrated to `CanonicalType`, new feature work should not extend the old `TypeSpec`-based path for that consumer
- duplicated semantic reconstruction code should be removed once the canonical path is proven equivalent
- parser-only responsibilities stay on `TypeSpec`
- sema/lowering semantic responsibilities move to `CanonicalType`

This means the project should not stabilize in a hybrid state where both of these remain true indefinitely:

- `TypeSpec` is used as semantic truth
- `CanonicalType` is also used as semantic truth

That would recreate the exact ambiguity this refactor is meant to remove.

## End-state responsibility split

### `TypeSpec` should keep

- parser-facing declarator shape
- syntactic qualifier/base information needed during parsing
- temporary compatibility support for legacy code during migration

### `TypeSpec` should lose over time

- being the authoritative function prototype carrier after sema
- being the authoritative callable-type representation after sema
- lowering-side indirect-call signature recovery
- mangling-facing type identity duties
- semantic reconstruction of pointer/function/array composition in downstream passes

### `CanonicalType` should gain

- declaration resolved type identity
- expression resolved type identity
- authoritative function and function-pointer prototype information
- callable detection and callable signature access
- semantic type comparison hooks
- canonical symbol and mangling input

## Deletion policy for overlapping logic

When a subsystem is migrated, the old `TypeSpec`-based semantic path should be intentionally retired instead of left in parallel indefinitely.

Recommended order:

1. Introduce canonical path for a subsystem
2. Prove parity using existing tests
3. Switch the subsystem to canonical-path-by-default
4. Delete redundant `TypeSpec` semantic reconstruction for that subsystem

Examples:

- once indirect call lowering reads canonical callable type, local `FnPtrSig` reconstruction should be reduced or removed there
- once function prototype compatibility checks read canonical signatures, duplicate `TypeSpec`-based compatibility assembly should be reduced or removed there
- once mangling consumes canonical symbols, it must not inspect parser-only declarator fields directly

This deletion step is not optional.

Without it, the codebase would accumulate two semantic type systems that drift apart over time.

## Suggested internal layering

### Layer 1: parser declarator layer

Owns:

- `TypeSpec`
- declarator parsing
- parameter node storage
- function-pointer parse-time shape

### Layer 2: sema canonicalization layer

Owns:

- `canonicalize_type(...)`
- `canonicalize_declarator_type(...)`
- resolved canonical types for declarations
- resolved canonical types for expressions later
- canonical function signature extraction

### Layer 3: sema consumers

Should gradually move to canonical type for:

- function compatibility checks
- prototype checks
- callable detection
- indirect call validation
- symbol identity construction

As each consumer migrates, the corresponding semantic `TypeSpec` path should be marked transitional and then removed.

### Layer 4: lowering/codegen consumers

Should gradually derive:

- function pointer signature info
- indirect call LLVM signature
- symbol prototype info

from canonical type rather than parser-only metadata.

## What must not happen

- no new major feature should deepen dependence on flattened `TypeSpec` flags if canonical type can carry the same meaning
- no lowering stage should need to reconstruct prototypes from multiple AST fields once canonical type is available
- no C++ mangling work should proceed on top of parser-only type identity
- no second independent semantic type system should be introduced beside canonical type

## Execution Plan

### Phase 1: introduce recursive canonical type model

- define recursive `CanonicalType`
- define canonical function signature representation
- define conversion from existing parser type artifacts into canonical type
- keep this available as a sema utility without changing behavior

Deliverable:

- canonical type exists as a compileable semantic model
- top-level functions, globals, arrays, pointers, and function pointers can be represented structurally

### Phase 2: introduce resolved type context

- add a sema-owned resolved type table or equivalent context
- record canonical type for declarations
- prepare extension point to record canonical type for expressions
- thread this through `AnalyzeResult` or equivalent sema result object
- document which existing `TypeSpec` consumers are now considered legacy bridges

Deliverable:

- sema can expose authoritative canonical types to later stages

### Phase 3: migrate callable/prototype logic

- convert function prototype extraction to use canonical type
- convert function pointer declaration handling to use canonical type
- convert indirect call analysis to use canonical type
- ensure unspecified-params and variadic information survive migration
- stop adding new callable/prototype behavior to `TypeSpec`-only paths

Deliverable:

- callable analysis no longer depends on ad hoc parser-side reconstruction

### Phase 4: migrate lowering hooks

- replace lowering-side function-signature reconstruction with canonical type access
- use canonical type for indirect call lowering
- reduce dependence on duplicated `FnPtrSig`-style side structures where possible
- delete redundant lowering-side `TypeSpec` prototype recovery once parity is established

Deliverable:

- lowering consumes sema-resolved callable type data

### Phase 4 detail: which `TypeSpec` duties must be removed from lowering

The end of Phase 4 should explicitly retire the following post-sema `TypeSpec` responsibilities from lowering/codegen paths:

- reconstructing function return types from `TypeSpec::is_fn_ptr`
- reconstructing callable signatures from `Node::fn_ptr_params`
- using `ptr_level` plus `is_fn_ptr` as the main way to distinguish object pointers from callable pointers
- maintaining parallel lowering-only function prototype side channels when the same information is already available in canonical type
- falling back to ad hoc call-signature heuristics where canonical callable type is available

Concretely, the migration should target logic shaped like:

- local/global/param `FnPtrSig` reconstruction from declaration nodes
- call-result inference that peels `TypeSpec` flags to guess callable return type
- indirect-call lowering that depends on declaration-specific side tables instead of resolved callable type

The intended end state is:

- lowering asks sema for canonical callable type
- lowering reads function return type and parameter list from canonical type
- `TypeSpec` no longer acts as the authoritative source of prototype information once sema is complete

### Phase 5: connect canonical symbol building

- build canonical declaration identity from sema-resolved canonical types
- ensure function prototypes and function definitions map to the same canonical function identity
- prepare template declaration vs instantiation distinction without requiring full template semantics yet
- forbid parser-only type fragments from acting as mangling input

Deliverable:

- symbol identity is now type-driven instead of parser-fragment-driven

### Phase 6: enable mangling on top of canonical symbols

- define mangler input purely in terms of canonical symbol and canonical type
- keep `extern "C"` as a bypass path
- reserve vendor extension mechanism for non-standard types like `fp8`

Deliverable:

- mangling can evolve independently from parser declarator quirks

## Acceptance Criteria For The Refactor

The refactor is successful when:

- canonical type is recursive
- sema owns canonical type construction
- declarations can expose canonical type after sema
- function prototype information is representable in canonical type
- function pointer declarations resolve to pointer-to-function canonical type
- indirect call logic can consume canonical type instead of parser-only fields
- existing C behavior remains unchanged
- the architecture is ready for canonical symbol and mangling work
- `TypeSpec` no longer acts as a second post-sema semantic truth for migrated subsystems

## Non-Goals

This refactor plan does not yet implement:

- full C++ template semantics
- overload resolution
- reference types
- member function qualifiers
- method pointers
- final ABI-compatible mangling
- complete replacement of every `TypeSpec` user in one pass

It only creates the semantic type architecture needed so those features can be added on top of a stable foundation.

## Agent Execution Scope

The next implementation pass should focus primarily on:

- [src/frontend/sema/canonical_symbol.hpp](/workspaces/c4c/src/frontend/sema/canonical_symbol.hpp)
- [src/frontend/sema/canonical_symbol.cpp](/workspaces/c4c/src/frontend/sema/canonical_symbol.cpp)

These files already exist and currently compile.

Current status:

- the recursive canonical type skeleton is present
- the code is buildable
- the canonical path is not yet fully integrated into the main sema/lowering flow

This means the agent should treat the current implementation as a compileable foundation rather than a finished integration.

Recommended focus for that pass:

- improve the recursive canonical model
- connect more declarator/prototype cases into canonical construction
- prepare resolved-type handoff points for later sema and lowering migration
- avoid broad unrelated refactors outside the canonical-type path unless required for compilation

## Final Design Rule

The most important rule is:

- parser describes how a type was written
- sema defines what the type is

If later stages still need to reconstruct semantic callable identity from parser-only fragments after this refactor, the structure is wrong and should be corrected before further C++ feature work continues.
