# 715-Guided BIR Core Redesign And LIR Import Migration Runbook

Status: Active
Source Idea: ideas/open/730_post_legacy_bir_shell_bootstrap.md

## Purpose

Build a bounded real BIR core from the accepted 715 contract, expose it through
`bir.hpp`, and migrate the retained LIR-to-BIR families onto it without
compiling or transplanting legacy BIR/prealloc.

## Goal

Land owner/generation-aware identities, separate storage and order, semantic
nodes, terminator-only CFG authority, builders, RawBir publication, and staged
verification; then migrate every existing `lir_to_bir` family incrementally and
connect the supported published view to a safe BIR-to-MIR boundary.

## Design Authority

- `docs/backend/pass_ready_bir/03_pass_ready_bir_contract.md`
- `docs/backend/pass_ready_bir/05_target_schema_and_api_blueprint.md`

Use these as constraints and hints.  Do not implement their complete P0--P13
roadmap in this runbook.

## Core Rule

`bir.hpp` is a facade over real core ownership.  No legacy API transplantation,
always-empty fake, duplicated CFG authority, or prepared/target state may enter
published BIR.

## Read First

- `ideas/open/730_post_legacy_bir_shell_bootstrap.md`
- the two design-authority documents above
- `src/backend/backend.hpp`
- every file under `src/backend/bir/lir_to_bir/`
- `src/backend/legacy/` only as quarantined historical reference

## Current State

- Step 1 completed at `793eeeb90`.
- CMake generation succeeds and compile commands contain no legacy path.
- Default build first fails because `src/backend/backend.hpp` includes missing
  `bir/bir.hpp`.
- Obsolete backend BIR/internal test registrations have been removed.

## Non-Goals

- Do not compile/copy/wrap/re-export `src/backend/legacy/**`.
- Do not implement editor/RAUW/analysis manager/canonical pass infrastructure
  unless a concrete migration packet proves a minimal dependency.
- Do not revive prealloc, prepared BIR, target planning, emission, or runtime
  authority.
- Do not silently convert unsupported meaningful LIR into empty BIR/MIR.

## Execution Rules

- Complete and review the schema/API checkpoint before broad implementation.
- Add only the schema/API surface needed by the current migration family.
- Construct through builders; publish through a move-only RawBir wrapper and
  read-only views.
- Grow verifier rules with each family before declaring it supported.
- Build after every schema or family seam and run its direct boundary proof.
- Reject not-yet-migrated forms safely and observably.
- Keep durable tests limited to direct LIR-to-new-BIR and new-BIR-to-MIR.
- Keep `src/backend/legacy/**` absent from all compile entries.

## Step 1: Restore CMake generation without legacy sources — Complete

Completed at: `793eeeb90`

Evidence:

- Removed obsolete prepared-BIR, prealloc, route, semantic-BIR, and
  `backend_lir_to_bir_notes` target/registration blocks.
- CMake generation succeeds.
- `build/compile_commands.json` contains no `src/backend/legacy/` entry.
- Default build reaches the missing `bir/bir.hpp` production seam.

## Step 2: Freeze the bounded schema and API checkpoint

Goal: Translate the 715 authority documents into the smallest implementation
contract that can support ordered LIR import migration.

Concrete actions:

- Specify the initial core file split and dependency direction behind the
  `bir.hpp` facade.
- Specify owner/generation layouts for `FunctionId`, `BlockId`, `InstId`, and
  `ValueId`; add module/global/local IDs only with a named migration need.
- Specify slot/generation storage, tombstones, and separate function/block/
  instruction ID order lists.
- Specify `Value`, `Inst`, `Block`, `Function`, and `Module` responsibilities,
  with terminators as sole successor authority.
- Specify builder-only construction, RawBir move-only publication, read-only
  views, and the staged verifier profiles needed by the first families.
- Record editor/RAUW/analysis/canonical-pass pieces as later 715 follow-ups
  unless a current family proves a minimal requirement.

Completion check:

- Reviewer can trace every proposed initial type/API to the authority docs and
  see that the checkpoint is sufficient for migration without implementing the
  full roadmap.

## Step 3: Implement core identity, ownership, order, and facade

Goal: Restore the missing production facade with a real minimal core.

Concrete actions:

- Implement generation/owner-aware stable IDs and checked storage/tombstones.
- Implement separate allocation ownership and semantic iteration order.
- Implement the bounded `Value`, `Inst`, `Block`, `Function`, and `Module`
  nodes from Step 2.
- Implement terminator-only successor representation.
- Make `bir.hpp` a narrow facade over core headers and read-only public views.
- Add no legacy, route, prealloc, prepared, target, printer, or debug fields.
- Build immediately and record the next true seam.

Completion check:

- Default build passes this facade/core seam, stable identity is not positional,
  CFG authority is singular, and compile metadata remains legacy-free.

## Step 4: Implement builders, RawBir publication, and foundation verification

Goal: Make valid core state constructible and publishable without exposing
mutable storage.

Concrete actions:

- Implement bounded `ModuleBuilder`/`FunctionBuilder` operations required by
  the import spine.
- Publish owned storage through a move-only `RawBir` result/wrapper with
  read-only views.
- Verify owner/generation validity, unique ownership, order membership,
  required terminators, and absence of unresolved builder tokens.
- Reject forbidden legacy/route/prealloc/prepared/target/printer/debug
  authority at publication.
- Build and add packet-local proof; keep durable tests only through a retained
  interface.

Completion check:

- Builders can publish a verified real RawBir core and invalid foundational
  states fail structurally before publication.

## Step 5: Migrate the import spine and CFG publication

Goal: Establish direct LIR-to-RawBir construction and terminator-derived CFG.

Migration order:

1. `lowering.hpp`, `context.cpp`, `module.cpp`, `types.cpp`
2. `cfg.cpp`

Concrete actions:

- Route construction through the new builders and eliminate old BIR API
  dependencies.
- Add only types, attributes, instructions, and verification rules required by
  these files.
- Make CFG successors derive solely from published terminators.
- Safely reject all not-yet-migrated semantic forms.
- Build after the import-spine packet and again after CFG; run direct
  LIR-to-new-BIR proof for each accepted subset.

Completion check:

- The import spine and CFG family publish verified RawBir through the new core,
  with exact safe rejection for the remaining inventory.

## Step 6: Migrate globals, scalar, and aggregate families

Goal: Expand supported RawBir semantics through three ordered packets.

Migration order:

1. `globals.cpp`, `global_initializers.cpp`
2. `scalar.cpp`
3. `aggregate.cpp`

Concrete actions:

- Add only the module IDs, values, opcodes, attributes, and verifier rules
  required by the current packet.
- Construct exclusively through builders and preserve owner/order invariants.
- Build and run direct LIR-to-new-BIR proof after each numbered packet.
- Keep later families on explicit safe rejection paths.

Completion check:

- All three family packets publish verified semantic nodes, with no legacy
  fallback or unverified accepted form.

## Step 7: Migrate memory semantic families

Goal: Bring memory import onto the new core without importing prepared/target
authority.

Migration order:

1. `memory/memory_types.hpp`, `memory/memory_helpers.hpp`,
   `memory/local_slots.cpp`
2. `memory/addressing.cpp`, `memory/provenance.cpp`
3. `memory/value_materialization.cpp`, `memory/local_gep.cpp`
4. `memory/intrinsics.cpp`, `memory/coordinator.cpp`

Concrete actions:

- Add local IDs only if required by the first packet and keep address/provenance
  facts semantic rather than target/prepared plans.
- Add operand/result/type/ownership verification with each packet.
- Build and run direct LIR-to-new-BIR proof after every numbered packet.
- Reject unmigrated memory forms without silent value creation or legacy lookup.

Completion check:

- Every listed memory file constructs verified core semantics in order, and no
  target allocation/address realization authority enters RawBir.

## Step 8: Migrate calls and remaining import analysis

Goal: Complete the retained source-family inventory.

Migration order:

1. `calling.cpp`, `call_abi.cpp`
2. `analysis.cpp`

Concrete actions:

- Keep source calling-convention semantics in core while excluding computed
  register/stack/call-move plans.
- Ensure import analysis remains adapter-local or derived; it must not become
  duplicated semantic authority in RawBir.
- Extend staged verification for signatures, call operands/results, ownership,
  and accepted attributes.
- Build and run direct LIR-to-new-BIR proof after each packet.

Completion check:

- Every retained `lir_to_bir` file is migrated or has an explicit recorded
  rejection boundary, and no legacy/prealloc API is compiled.

## Step 9: Connect the supported Raw/verified view to BIR-to-MIR

Goal: Establish the second retained interface without reviving preallocation.

Concrete actions:

- Define a bounded consumer entry for the supported published Raw/verified
  view.
- Accept only semantics whose lowering contract is implemented in this route.
- Reject unsupported non-empty semantics clearly and safely.
- Add direct new-BIR-to-MIR tests for accepted and rejected boundaries.
- Do not add prepared/prealloc/route/target authority to core BIR.

Completion check:

- The supported view reaches MIR through a direct tested boundary, unsupported
  semantics do not masquerade as success, and prealloc remains quarantined.

## Step 10: Validate the migrated surface and record follow-ups

Goal: Prove repository health and leave the remainder of the 715 roadmap
explicitly outside this bootstrap.

Concrete actions:

- Run the default configure/build after final source and build-graph changes.
- Run non-empty direct LIR-to-new-BIR and new-BIR-to-MIR test groups.
- Run broader CTest with output-on-failure.
- Confirm compile metadata contains no `src/backend/legacy/**` entry.
- Review published core for forbidden route/prealloc/prepared/target/printer/
  debug authority and duplicated CFG state.
- Record editor/RAUW/analysis manager/canonical passes and other unimplemented
  P0--P13 items as later 715-guided ideas, not hidden incomplete bootstrap work.

Completion check:

- Default build and broader CTest are reviewed, both interfaces have direct
  proof, legacy remains reference-only, and deferred 715 scope is explicit.
