# LinkNameId Backend Symbol Authority Runbook

Status: Active
Source Idea: ideas/open/162_link_name_id_backend_symbol_authority.md

## Purpose

Make final link-visible symbol identity flow through `LinkNameId` from HIR into LIR, BIR, and backend preparation whenever complete symbol metadata is available.

## Goal

Covered function, global, extern, direct-call, and initializer symbol paths should carry and consume `LinkNameId`; raw strings should remain output text, diagnostics, route-local names, or explicitly documented compatibility bridges.

## Core Rule

Do not make rendered names, `mangled_name`, LLVM spelling, printer text, or testcase-shaped string matching the semantic authority for covered symbols when a valid `LinkNameId` exists.

## Read First

- Source idea: `ideas/open/162_link_name_id_backend_symbol_authority.md`
- HIR symbols: `src/frontend/hir/hir_ir.hpp`
- HIR construction and lookup: `src/frontend/hir/hir_types.cpp`
- LIR IR: `src/codegen/lir/ir.hpp`
- HIR to LIR lowering: `src/codegen/lir/hir_to_lir/`
- BIR IR: `src/backend/bir/bir.hpp`
- LIR to BIR lowering: `src/backend/bir/lir_to_bir/`
- BIR validation and printing: `src/backend/bir/bir_validate.cpp`, `src/backend/bir/bir_printer.cpp`
- Backend preparation: `src/backend/prealloc/`

## Current Scope

- Inventory link-visible symbol string paths across HIR, LIR, BIR, and backend preparation.
- Propagate existing `LinkNameId` metadata through final function/global/direct-call/extern/initializer routes.
- Prefer `LinkNameId` for covered extern dedup, validation, and backend lookup.
- Fail closed or report structured mismatch when a covered known symbol has a complete `LinkNameId` miss.
- Add focused tests that prove structured symbol identity survives HIR to LIR to BIR.
- Document retained raw-name bridges with owner, limitation, and removal condition.

## Non-Goals

- Do not rework HIR template binding keys from idea 161.
- Do not redesign ABI mangling or intentionally change emitted symbol spelling except to intern an already-produced spelling.
- Do not replace SSA names, local slots, block labels, registers, string-pool labels, or inline asm text with `LinkNameId`.
- Do not remove all raw strings from printers, dumps, diagnostics, or textual backend output.
- Do not weaken tests, make unsupported expectation downgrades, or claim progress through output-only expectation rewrites.

## Working Model

- HIR owns final symbol materialization and interning into `Module::link_names`.
- LIR owns structured low-level transport for known link-visible symbols.
- BIR and backend preparation own validation, ABI preparation, and final spelling resolution from `LinkNameId`.
- Raw strings can remain as spelling payloads or compatibility bridges only when no complete metadata is available.

## Execution Rules

- Keep each step behavior-preserving except where the source idea explicitly requires fail-closed behavior for covered known-symbol misses.
- Prefer small slices that move one symbol route from string authority to `LinkNameId` authority with matching proof.
- When touching a raw-name fallback, classify it as output, route-local, diagnostic, or compatibility before changing it.
- A valid `LinkNameId` miss must not silently reopen raw-string lookup for a covered symbol path.
- Tests must prove structured symbol transport or collision prevention, not only changed dump text.
- Each code-changing step needs fresh build or targeted test proof; escalate to broader validation once multiple backend routes have changed.

## Ordered Steps

### Step 1: Inventory and Classify Link-Visible Symbol Paths

Goal: identify current raw-string authority and compatibility bridges before changing behavior.

Primary targets:
- `src/frontend/hir/hir_ir.hpp`
- `src/frontend/hir/hir_types.cpp`
- `src/codegen/lir/ir.hpp`
- `src/codegen/lir/hir_to_lir/`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/lir_to_bir/`
- `src/backend/bir/bir_validate.cpp`
- `src/backend/bir/bir_printer.cpp`
- `src/backend/prealloc/`

Actions:
- Inspect uses of `mangled_name`, `name`, `callee_name`, `init_text`, raw global names, extern names, and initializer symbol strings.
- Classify each relevant path as semantic authority, output spelling, route-local name, diagnostic/dump text, or compatibility bridge.
- Record retained compatibility bridges in `todo.md` with owner, limitation, and removal condition.
- Choose the first narrow implementation route where `LinkNameId` already exists but raw string still acts as authority.

Completion check:
- `todo.md` names the first implementation target and lists the raw-name bridges that should remain untouched for now.

### Step 2: Tighten HIR Symbol Materialization and Lookup Handoff

Goal: ensure HIR constructs and hands off valid `LinkNameId` for covered final symbols when metadata is complete.

Primary targets:
- `src/frontend/hir/hir_ir.hpp`
- `src/frontend/hir/hir_types.cpp`

Actions:
- Inspect `HirTemplateInstantiation::mangled_name` and `mangled_link_name_id`.
- Inspect `Function::link_name_id`, `GlobalVar::link_name_id`, `DeclRef::link_name_id`, and `Module::link_names`.
- Update function/global/template instantiation handoff paths to preserve existing `LinkNameId` alongside rendered spelling.
- Prefer existing `LinkNameId` in `make_function_lookup_decl_ref`, `make_global_lookup_decl_ref`, template call resolution, struct method link-name lookup, and global declaration lowering when metadata is complete.
- Keep rendered strings available for dumps, diagnostics, and emitted spelling.

Completion check:
- HIR direct call/global reference paths can reach lowering with valid `LinkNameId` where final symbol metadata exists.
- Narrow frontend or lowering tests/build proof covers the touched route.

### Step 3: Propagate LinkNameId Through HIR to LIR

Goal: make LIR receive structured identities for covered functions, globals, externs, direct calls, and initializer references.

Primary targets:
- `src/codegen/lir/ir.hpp`
- `src/codegen/lir/hir_to_lir/`

Actions:
- Verify `LirFunction::link_name_id`, `LirGlobal::link_name_id`, `LirExternDecl::link_name_id`, `LirCall::direct_callee_link_name_id`, `initializer_function_link_name_ids`, and `LirSpecEntry`.
- Update call target selection, global emission, extern declaration recording, and constant initializer emission to propagate `LinkNameId`.
- Make extern declaration dedup prefer `LinkNameId` when valid.
- Keep raw names as final spelling payloads and compatibility fields where no metadata exists.

Completion check:
- LIR dumps or tests show covered direct calls/globals/externs/initializer references retain `LinkNameId`.
- Targeted build and relevant LIR lowering tests pass.

### Step 4: Propagate LinkNameId Through LIR to BIR

Goal: make BIR consume the structured identities produced by LIR.

Primary targets:
- `src/backend/bir/bir.hpp`
- `src/backend/bir/lir_to_bir/`

Actions:
- Verify `Function::link_name_id`, `Global::link_name_id`, `CallInst::callee_link_name_id`, `LoadGlobalInst::global_name_id`, `StoreGlobalInst::global_name_id`, `MemoryAddress::base_link_name_id`, and `Global::initializer_symbol_name_id`.
- Update direct call lowering, global load/store lowering, memory-address global bases, function/global definitions, and initializer symbol references to keep `LinkNameId`.
- Treat invalid `LinkNameId` plus raw string as an explicit compatibility/no-metadata path, not the normal covered-symbol route.

Completion check:
- BIR for covered calls, globals, memory addresses, and initializer symbols contains valid `LinkNameId`.
- Targeted BIR lowering tests and build proof pass.

### Step 5: Add BIR Pointer Value Identity for Symbol-Carrying Values

Goal: close the remaining raw pointer-value bridges that cannot be fixed by
validation or backend preparation alone.

Primary targets:
- `src/backend/bir/bir.hpp`
- `src/backend/bir/lir_to_bir/`
- `src/backend/bir/bir_validate.cpp`
- `src/backend/prealloc/`

Actions:
- Add a structured identity carrier for BIR pointer values that represent
  link-visible function or global addresses, or add an equivalent address /
  provenance field consumed before backend preparation.
- Preserve `LinkNameId` for direct function pointer arguments, direct pointer
  stores, aggregate initializer pointer elements, byval aggregate pointer
  arguments, and synthesized pointer-array selections when complete metadata is
  available.
- Keep plain pointer values for route-local pointers, locals, slots, nulls, and
  unresolved compatibility/no-metadata paths.
- Avoid making printed `@symbol` text the semantic lookup key for covered
  pointer-valued symbol references.
- Add focused proof for at least one pointer-valued function reference and one
  aggregate or byval pointer route that previously only carried raw spelling.

Completion check:
- Covered pointer-valued symbol references either carry structured identity or
  are explicitly classified as compatibility/no-metadata paths.
- Step 6 can validate/backend-prepare these routes without needing raw symbol
  spelling as the normal authority.
- Targeted BIR lowering or backend tests and build proof pass.

### Step 6: Make BIR Validation and Backend Preparation LinkNameId-Authoritative

Goal: validate and prepare covered known symbols by `LinkNameId` first and fail closed on complete metadata mismatches.

Primary targets:
- `src/backend/bir/bir_validate.cpp`
- `src/backend/bir/bir_printer.cpp`
- `src/backend/prealloc/`

Actions:
- Update call/global validation to prefer `LinkNameId`.
- Ensure backend preparation resolves final assembly or IR spelling from `LinkNameId` where available.
- Make a valid `LinkNameId` miss produce a structured failure instead of silently falling back to raw string lookup.
- Keep printer output and diagnostics textual, but prevent them from becoming semantic lookup authority.

Completion check:
- Covered known-symbol validation rejects mismatched or missing `LinkNameId` routes.
- Backend preparation still emits expected final spelling by resolving through the name table.
- Targeted backend tests and build proof pass.

### Step 7: Add Focused Identity Tests and Broader Proof

Goal: prove structured identity survives the final backend route and raw-name compatibility cannot override it.

Actions:
- Add tests where rendered-compatible or same-spelled raw symbol text would collide without interned link identity.
- Add tests proving direct calls/globals keep `LinkNameId` through HIR to LIR to BIR.
- Include a negative or mismatch-oriented test where a complete `LinkNameId` miss does not reopen raw string lookup.
- Run the supervisor-selected broader validation after multiple symbol routes have changed.

Completion check:
- Focused tests fail for raw-string authority and pass with `LinkNameId` authority.
- Broader validation passes with no unsupported expectation downgrades or output-only proof.

### Step 8: Document Retained Compatibility Bridges

Goal: make remaining raw-name bridges explicit and reviewable.

Actions:
- For every retained raw-name compatibility path, document owner, limitation, and removal condition near the relevant code or in the execution notes if no code comment is warranted.
- Confirm route-local textual names remain strings and are not forced into `LinkNameId`.
- Confirm source idea acceptance criteria are satisfied or record remaining work before closure review.

Completion check:
- Remaining raw string paths are either output, diagnostics, route-local names, or documented compatibility bridges.
- `todo.md` has enough proof notes for the supervisor to decide whether to close, continue, or request review.
