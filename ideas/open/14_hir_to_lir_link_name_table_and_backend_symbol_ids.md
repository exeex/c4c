# HIR To LIR Link-Name Table And Backend Symbol Ids

## Goal

Introduce a dedicated `LinkNameId` table at the HIR-to-LIR boundary so final
link-visible logical names stop flowing through the pipeline as ad hoc strings.

The intended end state is:

- parser/source ingestion owns source-atom identity such as `foo`, `std`, and
  `vector`
- HIR owns the first materialization of final logical symbol names for emitted
  functions/globals/template instantiations
- HIR and LIR carry stable `LinkNameId` values rather than raw symbol strings
  for link-visible names
- later consumers such as LLVM text emission or backend assembly emission
  resolve `LinkNameId` back to text only at the final spelling boundary

## Why This Is Separate

The repo is already converging on a cleaner pattern for cross-stage contracts:
move from stringly-typed or implicit conventions to explicit structured data.

Recent work did that for targets:

- frontend and backend now prefer `TargetProfile`
- HIR no longer stores `target_triple`
- LIR no longer treats the incoming triple string as its contract
- LLVM-facing triple text is now generated canonically from `TargetProfile`

Final symbol names have the same shape of problem.

Today the pipeline already has a de facto late-bound name layer:

- HIR stores final function/global names as strings
- HIR template instantiations store mangled names as strings
- LIR direct-call/global/extern carriers store logical names as strings
- backend helpers consume those strings to derive target-spelled symbols

That works, but it leaves a real contract implicit. Final logical names are not
the same thing as parser-time identifier atoms, and they should not remain
unstructured text if they are meant to survive through HIR, LIR, backend
lowering, and final emission.

## Core Design

Define a second, later-closing id space:

- `SymbolId` for source-level identifier atoms
- `LinkNameId` for final logical symbol names used by emitted code

These are intentionally different tables even when the underlying text matches.

The semantic split is:

- `SymbolId("foo")` means source-atom identity
- `LinkNameId("foo")` means final logical symbol identity for emitted code

## Core Lifecycle Rules

- `SymbolId` space is built during lexing/source ingestion and then frozen
- `LinkNameId` space is built when HIR materializes final logical names and
  then frozen
- HIR is the first stage allowed to intern final logical symbol names
- LIR and later stages consume `LinkNameId` rather than raw symbol strings
  wherever the data represents a final link-visible name
- backend-private temporary names are out of scope for the first slice

## Data Model

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

- source-atom symbol tables
- raw token/text tables
- file tables

## Correct Boundary

HIR is the right place to materialize `LinkNameId`.

At that point the frontend already knows the final logical name for emitted
entities such as:

- ordinary function names
- ordinary global names
- qualified namespace-level names
- anonymous-namespace synthesized names
- operator canonical names
- template mangled names
- externally referenced function/global names

That makes HIR the first stage where "this is the final logical symbol name
consumed by later codegen/backend layers" is actually true.

## Important Review Corrections

This open idea intentionally tightens a few parts of the original draft.

### 1. Do Not Lead With `[[deprecated]]`

The draft leaned heavily on early warning-driven deprecation of legacy string
fields. That is probably too aggressive as a first packet.

The first landing should instead prefer:

- parallel `link_name_id` fields
- explicit `LinkNameTable` threading
- targeted consumer migration

Deprecation can come after the id path is real and at least one full HIR->LIR
path uses it successfully.

### 2. The Table Must Travel With The Boundary

This cannot be only a lowering-context side channel.

If later stages are supposed to consume `LinkNameId`, then the table lookup
surface must remain available at the module or lowering-boundary level.

A good first rule is:

- HIR module owns or references the canonical `LinkNameTable`
- HIR->LIR lowering forwards the same logical-name space
- later consumers that need text lookup use that same table rather than
  reconstructing names ad hoc

### 3. Final String Lookup Happens At The Last Consumer Boundary

The backend is the most obvious consumer of final symbol strings, but it is not
the only one. If LLVM textual emission still happens from LIR, that printer is
also a late consumer boundary.

So the correct rule is:

- HIR/LIR/intermediate lowering carry `LinkNameId`
- the last text-emitting consumer resolves `LinkNameId` to text
- target-specific decoration such as Darwin leading `_` remains backend-only

### 4. Keep Backend-Private Names Out Of Scope

The first slice should not absorb:

- SSA temp names
- block labels
- stack-slot names
- rewrite/debug helper names
- string-pool/private backend labels unless they are already treated as
  link-visible logical symbols

## First Implementation Scope

### 1. Add The Table And Id Type

- define `LinkNameId`
- define `LinkNameTable`
- add a clear invalid sentinel

### 2. Extend HIR Symbol Carriers

Primary files:

- [src/frontend/hir/hir_ir.hpp](/workspaces/c4c/src/frontend/hir/hir_ir.hpp)
- [src/frontend/hir/hir_build.cpp](/workspaces/c4c/src/frontend/hir/hir_build.cpp)
- [src/frontend/hir/hir_functions.cpp](/workspaces/c4c/src/frontend/hir/hir_functions.cpp)
- [src/frontend/hir/hir_types.cpp](/workspaces/c4c/src/frontend/hir/hir_types.cpp)
- [src/frontend/hir/hir_lowerer_internal.hpp](/workspaces/c4c/src/frontend/hir/hir_lowerer_internal.hpp)

First HIR carriers to extend:

- `hir::Function`
- `hir::GlobalVar`
- `hir::HirTemplateInstantiation`
- HIR indexes keyed by final emitted symbol identity

The first landing can use parallel fields such as:

- `Function.link_name_id`
- `GlobalVar.link_name_id`
- `HirTemplateInstantiation.mangled_link_name_id`

### 3. Thread The Table Through HIR Ownership

Suggested ownership shape:

```cpp
struct LoweringTables {
  LinkNameTable* link_names = nullptr;
};
```

The point is not this exact type shape, but the rule that HIR symbol
materialization owns the interning boundary.

### 4. Forward Ids Into LIR

Primary files:

- [src/codegen/lir/ir.hpp](/workspaces/c4c/src/codegen/lir/ir.hpp)
- [src/codegen/lir/hir_to_lir.cpp](/workspaces/c4c/src/codegen/lir/hir_to_lir.cpp)

First LIR carriers to extend:

- direct-call callee names
- global symbol carriers
- extern declaration symbol names
- specialization metadata names

The first slice can keep bridging string fields temporarily, but the forward
direction should be explicit:

```cpp
out.callee_link_name_id = hir_call.callee_link_name_id;
```

### 5. Resolve At Final Text Boundaries

Likely early consumers:

- LLVM text emission from LIR
- backend symbol spelling helpers

The intended rule is:

1. lookup `LinkNameId` to obtain the platform-neutral logical name
2. apply target decoration only in the backend when target spelling differs
3. emit text/asm/object-facing names from that late consumer boundary

## Candidate Carriers

Likely HIR-side carriers:

- `hir::Function.name`
- `hir::GlobalVar.name`
- `hir::Module.fn_index`
- `hir::Module.global_index`
- `hir::HirTemplateInstantiation.mangled_name`
- final global/function references that already mean emitted-link identity

Likely LIR-side carriers:

- direct-call callee name
- `LirGlobal.name`
- `LirExternDecl.name`
- extern-decl map keys
- specialization metadata names

## Constraints

- do not merge `LinkNameId` into source-atom `SymbolId`
- do not treat parser/source spelling as the same contract as emitted-link
  identity
- do not force backend-private temporary names into the first slice
- do not push target-specific decoration into HIR or LIR
- do not let the first landing turn into a repo-wide string purge

## Acceptance Criteria

- [ ] HIR has an explicit `LinkNameId` / `LinkNameTable` concept for final
      logical symbol names
- [ ] at least one real HIR->LIR path forwards link-visible symbol identity as
      ids rather than only raw strings
- [ ] later text-emitting consumers can resolve `LinkNameId` through a stable
      lookup boundary
- [ ] target-specific symbol decoration still happens only at the backend/text
      spelling edge
- [ ] backend-private temporary names remain out of scope for the first slice

## Validation

- existing frontend/HIR regression suite
- HIR-to-LIR lowering coverage
- backend emission tests for ordinary function/global names
- template-instantiation and operator-name symbol tests
- anonymous namespace / qualified-name symbol tests
- cross-platform symbol-spelling checks where Darwin underscore decoration
  matters

## Non-Goals

- replacing all strings in HIR/LIR
- redesigning parser/source-atom symbol tables
- introducing a backend-private SSA-name table in the first slice
- requiring `LinkNameTable` and source-atom tables to deduplicate against each
  other

## Good First Patch

Add `LinkNameId` and `LinkNameTable`, thread them into HIR symbol
materialization, add parallel id fields for `hir::Function` and
`hir::GlobalVar`, then forward one bounded HIR->LIR direct-symbol path through
those ids without trying to rewrite every backend-private name at once.
