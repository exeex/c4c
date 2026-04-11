# HIR To LIR Link-Name Table And Backend Symbol Ids

Status: Draft
Last Updated: 2026-04-10

## Goal

Introduce a dedicated `LinkNameId` table for the HIR-to-LIR boundary so final
logical symbol names become stable ids before backend lowering, while keeping
that id space separate from source-atom `SymbolId`.

## Why This Idea Exists

The source-atom symbol-table idea solves parser-side identity for names such as
`std`, `vector`, and `foo`, but that is not the same thing as the name model
consumed by HIR, LIR, backend emission, and the final linker-facing symbol
surface.

By the time the frontend reaches HIR/LIR, the pipeline already relies on
materialized symbol strings for things such as:

- fully qualified function/global names
- anonymous namespace synthesized names such as `__anon_ns_N`
- operator canonical names
- template instantiation mangled names
- extern declaration names
- string-pool and helper symbol names

Those names are not source atoms, and they should not be forced into the same
id space as parser-time identifier atoms.

## Main Objective

Define a second, later-closing name-id space:

- `SymbolId` for source-level identifier atoms, closed after lexing
- `LinkNameId` for final logical symbol names, closed after HIR symbol
  materialization

The key design choice is that HIR becomes the boundary where final link-visible
logical names are materialized and interned into a dedicated `LinkNameTable`.
From that point forward, LIR and backend code consume `LinkNameId` rather than
raw symbol strings wherever the data represents a final logical symbol.

## Core Lifecycle Rules

- `SymbolId` space is built during lexing/source ingestion and then frozen.
- Parser and sema may read `SymbolId`, but do not extend the source-atom table
  after lex completion.
- `LinkNameId` space is built during HIR symbol materialization and then frozen.
- LIR, backend, and linker-facing logic consume `LinkNameId` only.
- `SymbolId` and `LinkNameId` are intentionally separate id spaces even when
  the underlying strings overlap.

## Why Separate Tables

Even if a final logical symbol name happens to equal a source atom spelling,
the two identities carry different semantics:

- `SymbolId("foo")` means source-level identifier atom identity
- `LinkNameId("foo")` means final logical symbol identity in emitted IR/object
  code

Keeping the spaces separate avoids:

- mixing composed/generated names into the source-atom table
- forcing parser concerns into backend naming design
- accidental assumptions that qualified or synthesized names are parser atoms

Duplicate strings across the two tables are acceptable and expected.

## Current Code Shape

Today HIR already stores final symbol names as strings:

- `hir::Function.name`
- `hir::GlobalVar.name`
- `hir::Module.fn_index`
- `hir::Module.global_index`
- `HirTemplateInstantiation.mangled_name`

Today LIR also stores final symbol names as strings:

- `LirCall.callee_name`
- `LirGlobal.name`
- `LirExternDecl` and `extern_decl_map`
- specialization metadata `mangled_name`
- string-pool names such as `@.str0`

The backend finally consumes those strings through helpers such as
`asm_symbol_name(target_triple, logical_name)`.

This means the pipeline already has a de facto "final logical symbol name"
layer. The missing piece is making that layer explicit and id-based.

## Proposed Data Model

```cpp
using LinkNameId = uint32_t;
constexpr LinkNameId kInvalidLinkName = 0;

class LinkNameTable {
 public:
  LinkNameId intern(std::string_view logical_name);
  std::string_view lookup(LinkNameId id) const;

 private:
  std::vector<std::string> text_by_id_;
  std::unordered_map<std::string, LinkNameId> id_by_text_;
};
```

This table is separate from:

- `SymbolInterner` for source atoms
- `TextTable` for raw token spelling
- `FileTable` for source file identity

## Concrete Injection Points

The first implementation should plug into these existing files rather than
inventing a separate side channel.

### 1. HIR name carriers

Primary file:

- [hir_ir.hpp](/workspaces/c4c/src/frontend/hir/hir_ir.hpp)

Current carriers to extend:

- `using SymbolName = std::string`
- `struct Function`
- `struct GlobalVar`
- `struct HirTemplateInstantiation`
- `Module::fn_index`
- `Module::global_index`

Suggested transitional shape:

```cpp
using LinkNameId = uint32_t;
constexpr LinkNameId kInvalidLinkName = 0;

struct Function {
  FunctionId id{};
  LinkNameId link_name_id = kInvalidLinkName;

  [[deprecated("use link_name_id via LinkNameTable")]]
  std::string name;

  ExecutionDomain execution_domain = ExecutionDomain::Host;
  ...

  [[deprecated("use template_origin_link_name_id where applicable")]]
  std::string template_origin;
};

struct GlobalVar {
  GlobalId id{};
  LinkNameId link_name_id = kInvalidLinkName;

  [[deprecated("use link_name_id via LinkNameTable")]]
  std::string name;

  ExecutionDomain execution_domain = ExecutionDomain::Host;
  ...
};

struct HirTemplateInstantiation {
  LinkNameId mangled_link_name_id = kInvalidLinkName;

  [[deprecated("use mangled_link_name_id")]]
  std::string mangled_name;

  TypeBindings bindings;
  NttpBindings nttp_bindings;
  SpecializationKey spec_key;
};
```

Suggested module-index migration:

```cpp
struct Module {
  ...
  std::unordered_map<LinkNameId, FunctionId> fn_index_by_link_name;
  std::unordered_map<LinkNameId, GlobalId> global_index_by_link_name;

  [[deprecated("use fn_index_by_link_name")]]
  std::unordered_map<std::string, FunctionId> fn_index;

  [[deprecated("use global_index_by_link_name")]]
  std::unordered_map<std::string, GlobalId> global_index;
};
```

### 2. HIR lowering ownership

Primary files:

- [hir_build.cpp](/workspaces/c4c/src/frontend/hir/hir_build.cpp)
- [hir_functions.cpp](/workspaces/c4c/src/frontend/hir/hir_functions.cpp)
- [hir_types.cpp](/workspaces/c4c/src/frontend/hir/hir_types.cpp)
- [hir_lowerer_internal.hpp](/workspaces/c4c/src/frontend/hir/hir_lowerer_internal.hpp)

These are the right places to inject a `LinkNameTable` reference into the
lowering context and to assign `link_name_id` when a function/global/final
instantiation name becomes real.

Suggested ownership shape:

```cpp
struct LoweringTables {
  LinkNameTable* link_names = nullptr;
};

class Lowerer {
  ...
  LoweringTables* tables_ = nullptr;
};
```

Suggested HIR symbol materialization helper:

```cpp
inline LinkNameId intern_link_name(LoweringTables& tables,
                                   const std::string& logical_name) {
  return tables.link_names->intern(logical_name);
}
```

### 3. LIR symbol carriers

Primary file:

- [ir.hpp](/workspaces/c4c/src/codegen/lir/ir.hpp)

Current carriers to extend:

- `LirCall.callee_name`
- `LirGlobal.name`
- `LirSpecEntry.template_origin`
- `LirSpecEntry.mangled_name`
- `LirModule.extern_decl_map`
- `LirExternDecl.name`

Suggested transitional shape:

```cpp
struct LirCall {
  LirValueId result{};
  TypeSpec return_type{};
  LinkNameId callee_link_name_id = kInvalidLinkName;

  [[deprecated("use callee_link_name_id")]]
  std::string callee_name;

  LirValueId callee_ptr{};
  std::vector<LirValueId> args;
};

struct LirGlobal {
  LirGlobalId id{};
  LinkNameId link_name_id = kInvalidLinkName;

  [[deprecated("use link_name_id")]]
  std::string name;

  TypeSpec type{};
  ...
};

struct LirSpecEntry {
  std::string spec_key;
  LinkNameId template_origin_link_name_id = kInvalidLinkName;
  LinkNameId mangled_link_name_id = kInvalidLinkName;

  [[deprecated("use *_link_name_id fields")]]
  std::string template_origin;

  [[deprecated("use *_link_name_id fields")]]
  std::string mangled_name;
};
```

Suggested extern-decl migration:

```cpp
struct LirExternDecl {
  LinkNameId link_name_id = kInvalidLinkName;

  [[deprecated("use link_name_id")]]
  std::string name;

  std::string ret_ty;
};

struct LirModule {
  ...
  std::unordered_map<LinkNameId, std::string> extern_decl_map_by_link_name;

  [[deprecated("use extern_decl_map_by_link_name")]]
  std::unordered_map<std::string, std::string> extern_decl_map;

  void record_extern_decl(LinkNameId name, const std::string& ret_ty) {
    auto it = extern_decl_map_by_link_name.find(name);
    if (it == extern_decl_map_by_link_name.end()) {
      extern_decl_map_by_link_name.emplace(name, ret_ty);
      return;
    }
    if (it->second == "void" && ret_ty != "void") it->second = ret_ty;
  }
};
```

### 4. HIR to LIR translation

Primary file:

- [hir_to_lir.cpp](/workspaces/c4c/src/codegen/lir/hir_to_lir.cpp)

This file is where HIR string symbol fields currently get copied into LIR.
It should become the main bridge that forwards `LinkNameId` instead.

Suggested translation style:

```cpp
LirCall out;
out.callee_link_name_id = hir_call.callee_link_name_id;
out.callee_name = link_names.lookup(out.callee_link_name_id); // temporary bridge only
```

The bridge lookup above is transitional. The goal is for downstream LIR/backend
code to stop reading `callee_name` directly.

### 5. Backend lookup boundary

Primary files:

- [emit.cpp](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
- [emit.cpp](/workspaces/c4c/src/backend/aarch64/codegen/emit.cpp)

Current backend helpers already have a clean final boundary:

```cpp
std::string asm_symbol_name(std::string_view target_triple,
                            std::string_view logical_name);
```

The new handoff should be:

```cpp
std::string logical_name = std::string(link_names.lookup(link_name_id));
std::string asm_name = asm_symbol_name(module.target_triple, logical_name);
```

That means target-specific underscore/prefix rules remain backend-only.

## Fields To Deprecate Early

To make warning-driven cleanup effective, the following existing string fields
should be marked `[[deprecated]]` early in the migration.

HIR:

- `hir::Function.name`
- `hir::Function.template_origin`
- `hir::GlobalVar.name`
- `hir::HirTemplateInstantiation.mangled_name`
- `hir::Module.fn_index`
- `hir::Module.global_index`

LIR:

- `LirCall.callee_name`
- `LirGlobal.name`
- `LirExternDecl.name`
- `LirSpecEntry.template_origin`
- `LirSpecEntry.mangled_name`
- `LirModule.extern_decl_map`

The deprecation policy should match the token-id idea: warnings are not treated
as a problem here, but as the primary mechanism for exposing legacy string-based
call sites quickly.

## First Landing Slice

The first implementation slice should stay narrow and mechanical.

1. Add `LinkNameId` and `LinkNameTable`.
2. Add parallel `link_name_id` fields to HIR `Function` and `GlobalVar`.
3. Add `fn_index_by_link_name` and `global_index_by_link_name`.
4. Mark legacy HIR string symbol fields deprecated.
5. Update HIR construction sites in:
   - [hir_functions.cpp](/workspaces/c4c/src/frontend/hir/hir_functions.cpp)
   - [hir_types.cpp](/workspaces/c4c/src/frontend/hir/hir_types.cpp)
   - [hir_build.cpp](/workspaces/c4c/src/frontend/hir/hir_build.cpp)
6. Thread `link_name_id` through [hir_to_lir.cpp](/workspaces/c4c/src/codegen/lir/hir_to_lir.cpp) into `LirCall` and `LirGlobal`.
7. Add deprecated bridge fields on the LIR side.

That slice is enough to prove the boundary without forcing immediate conversion
of every backend-private name.

## HIR Boundary

HIR is the correct place to materialize `LinkNameId`.

At this stage, the frontend already knows:

- the final selected function/global name
- namespace qualification
- anonymous namespace expansion
- operator canonical name
- template mangled name

That makes HIR the first stage where "this is the final logical symbol name for
backend consumption" is actually true.

The intended flow is:

1. parser/sema/lowering work with source names and source-atom ids
2. HIR lowering decides the final logical symbol name for each emitted entity
3. HIR interns that string in `LinkNameTable`
4. HIR stores `LinkNameId`
5. LIR/backend consume only `LinkNameId`

## Candidate HIR Changes

The following HIR fields are strong candidates to move from `std::string` to
`LinkNameId` or to carry a parallel `link_name_id` during migration:

- `hir::Function.name`
- `hir::GlobalVar.name`
- `hir::Module.fn_index`
- `hir::Module.global_index`
- `hir::HirTemplateInstantiation.mangled_name`
- `hir::ConstevalCallInfo.fn_name`
- HIR decl/global references that refer to final global/function names

Some source-facing metadata should remain textual or source-structured:

- namespace qualifier segments
- source display/debug names
- source spelling preserved for diagnostics

## Candidate LIR Changes

The following LIR fields are strong candidates to move from string names to
`LinkNameId`:

- `LirCall.callee_name`
- `LirGlobal.name`
- `LirExternDecl.name`
- `LirModule.extern_decl_map` keys
- `LirSpecEntry.template_origin`
- `LirSpecEntry.mangled_name`

String-pool labels and helper/private labels need a deliberate split:

- externally visible logical symbols should use `LinkNameId`
- backend-private labels may remain backend-local strings or gain a later
  backend-private id table

## Backend Boundary

The backend should be the first place that converts `LinkNameId` back into a
string for target-specific symbol spelling.

The intended backend rule is:

1. `lookup(link_name_id)` gives the platform-neutral logical symbol name
2. backend target code applies target conventions such as Darwin leading `_`
3. assembly/object emission uses the target-spelled name

This keeps the target-specific decoration out of HIR and LIR.

## Names In Scope For LinkNameId

The first `LinkNameId` slice should include final logical names such as:

- ordinary function names
- ordinary global names
- qualified namespace-level symbols
- anonymous namespace synthesized names
- operator canonical names
- template instantiation mangled names
- externally referenced function/global names

## Names Out Of Scope Initially

The first `LinkNameId` slice does not need to absorb every string in backend IR.

These can remain separate initially:

- raw token text
- parser-only composed names
- backend-private temporary SSA names
- backend-private block labels
- stack-slot/value rewrite names
- string-pool labels if keeping them backend-local is simpler for the first pass

## Migration Strategy

This migration should follow the same warning-driven style as the token-id
migration.

Suggested order:

1. add `LinkNameTable` to the HIR/LIR lowering context
2. add parallel `link_name_id` fields to the main HIR symbol carriers
3. keep legacy string fields temporarily and mark them deprecated where
   practical
4. update `fn_index/global_index`-style maps to key on `LinkNameId`
5. update LIR symbol carriers to use `LinkNameId`
6. update backend symbol emission to resolve through `LinkNameTable`
7. remove legacy string carriers once the pipeline is mostly id-based

## Validation

- existing frontend/HIR regression suite
- HIR-to-LIR lowering coverage
- backend emission tests for normal functions/globals
- backend emission tests for operator names and template instantiations
- anonymous namespace / qualified-name symbol tests
- cross-platform symbol spelling checks where Darwin underscore decoration
  matters

## Constraints

- do not merge `LinkNameId` into `SymbolId`
- do not force backend-private temporary names into the first link-name slice
- keep target-specific symbol decoration out of HIR and LIR
- keep the first migration focused on names that genuinely survive to linking

## Non-Goals

- no attempt to replace all textual metadata in HIR/LIR
- no redesign of parser source-atom symbol tables in this idea
- no backend-private SSA name table in the first slice
- no requirement that `LinkNameTable` and `SymbolTable` deduplicate against each
  other
