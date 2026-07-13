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
- Steps 2--4 completed across `a89624c62` through `07545736f`: the bounded
  schema checkpoint, stable identity/storage/order, read-only facade/views,
  scoped builders, move-only RawBir publication, and foundation verifier are
  present.
- Step 5 completed across
  `2e7d9ecb6` through `d03f45cda`: a verified minimal LIR-to-RawBir subset is
  active, legacy prealloc/MIR consumers are quarantined, the default build is
  green, and only retained interface tests are registered under backend.
- `2b6148590` closed the remaining active-source string-authority debt.  The
  guard excludes only quarantined `src/backend/legacy/**`, the bootstrap
  link-name uniqueness index has an exact bounded classification, and broader
  CTest is 3030/3030.
- Step 6.1 is the current bounded packet: module globals and their
  global-initializer semantics only.

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

## Step 2: Freeze the bounded schema and API checkpoint — Complete

Completed at: `a89624c62`

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

Evidence:

- `docs/backend/pass_ready_bir/07_bootstrap_schema_checkpoint.md` freezes the
  bounded file/type/API surface and incorporates the independent review
  repairs before implementation authority was accepted.
- The checkpoint explicitly keeps the link-name map as a bootstrap uniqueness
  and merge index, never stable entity identity.

## Step 3: Implement core identity, ownership, order, and facade — Complete

Completed across: `1827457d7`, `4fd588556`, `f5c6a6311`

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

Evidence:

- Owner/generation-aware IDs, checked slot storage/tombstones, and separate ID
  order landed without making positions or names semantic identity.
- `bir.hpp` is a grouped public reading map over bounded nodes and read-only
  views; mutable storage remains private.

## Step 4: Implement builders, RawBir publication, and foundation verification — Complete

Completed across: `b94c29ab2`, `07545736f`

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

Evidence:

- Scoped builders enforce mutation authority and gated publication.
- Foundation verification covers owner/generation/kind resolution,
  storage/order membership, function shape, terminators, and exact link-name
  index consistency before move-only RawBir publication.

## Step 5: Migrate the import spine and CFG publication — Complete

Completed across: `2e7d9ecb6` through `2b6148590`

Goal: Establish direct LIR-to-RawBir construction and terminator-derived CFG.

Migration order:

1. Bootstrap import/publication seam — Complete
2. Close active-source string-authority guard debt — Complete

Concrete actions:

- Keep the completed top-level `lower_lir_to_raw_bir()` path as the active
  import spine.  It accepts void declarations and ordered void definitions
  using return, unconditional branch, and unreachable terminators, constructs
  exclusively through builders, and derives successors only from terminators.
- Keep parameters, non-void signatures, globals, ordinary instructions, and
  unsupported terminators on structured rejection paths until their semantic
  family lands atomically.  Do not reactivate the excluded historical
  `src/backend/bir/lir_to_bir/` translation units merely to reuse their names.
- Restore `string_authority_guard` without classifying archived code:
  explicitly exclude quarantined `src/backend/legacy/**` from that active-code
  scanner while preserving the rest of `src/backend` coverage.
- Add one exact classification for
  `ModuleData::functions_by_link_name_` as the checkpoint-approved ABI/link
  spelling uniqueness and merge index.  The classification and adjacent code
  evidence must state that `FunctionId` remains entity identity, the verifier
  enforces bidirectional exactness, and the map is removable when BIR receives
  an owner-correct interned link identity.  Do not add a broad scanner
  exemption for active BIR and do not introduce a new ID family in this guard
  cleanup packet.
- Run the guard self-test and guard test, then the retained interface subset and
  broader CTest.  Advance to Step 6 only when this prerequisite is green.

Completion check:

- The import spine and CFG family publish verified RawBir through the new core,
  with exact safe rejection for the remaining inventory.
- Default build, retained LIR-to-BIR interface proof, and the broader suite are
  green without scanning/classifying archived legacy implementation as active
  authority.

Completed bootstrap evidence:

- `2e7d9ecb6` added the verified minimal importer and explicit rejection
  boundary.
- `b9fba968a` routed the backend consumer through that importer and quarantined
  prealloc/MIR consumers without fake emission success.
- `de5e0977d`, `b056af5a9`, and `d03f45cda` removed legacy build/test graph
  reachability and established the retained LIR-to-BIR interface test.
- `2b6148590` excluded only quarantined legacy sources from the active-code
  string scanner, classified the link-name uniqueness/merge index without
  making strings entity identity, and passed the guard self-tests, focused
  interface tests, and broader CTest 3030/3030.

## Step 6: Migrate globals, scalar, and aggregate families — In Progress

Goal: Expand supported RawBir semantics through three ordered packets.

Migration order:

1. Step 6.1: globals and global initializers — In Progress
2. Step 6.2: scalar instructions — Pending
3. Step 6.3: aggregate instructions — Pending

### Step 6.1: Globals and global initializers — In Progress

Primary behavior inventory:

- `src/backend/bir/lir_to_bir/globals.cpp`
- `src/backend/bir/lir_to_bir/global_initializers.cpp`

Concrete actions:

- Reimplement only module-level global declaration/definition identity,
  deterministic global order, attributes, and initializer references/payloads
  required by the LIR global records.
- Add the smallest `GlobalId`, owned global storage/order, read-only view,
  builder operations, and verifier rules needed to publish that family.
- Treat the retained importer files as behavior inventory only.  Do not compile
  them unchanged, transplant their legacy BIR representation, or restore a
  dependency on archived legacy/prealloc/target authority.
- Keep function-body scalar and aggregate instruction lowering on structured
  rejection paths.  Constant payload forms needed exclusively by a global
  initializer do not authorize migration of `scalar.cpp` or `aggregate.cpp`.
- Reject every initializer form not represented and verified by this packet;
  no unsupported form may publish success or silently become zero/empty data.
- Prove global declaration, accepted initializer, and rejected unsupported
  initializer behavior through the retained LIR-to-new-BIR interface.

Completion check:

- Module globals and the explicitly supported initializer subset publish with
  stable owner-correct identity and exact storage/order/reference verification.
- Scalar/aggregate function instructions still reject, compile metadata stays
  legacy/prealloc/MIR-free, and the selected build/interface proof is green.

### Step 6.2: Scalar instructions — Pending

Primary behavior inventory: `src/backend/bir/lir_to_bir/scalar.cpp`.

Do not begin until Step 6.1 is accepted and committed.

### Step 6.3: Aggregate instructions — Pending

Primary behavior inventory: `src/backend/bir/lir_to_bir/aggregate.cpp`.

Do not begin until Step 6.2 is accepted and committed.

Concrete actions:

- Add only the module IDs, values, opcodes, attributes, and verifier rules
  required by the current numbered packet.
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
