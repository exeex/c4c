# Backend Reboot From BIR Spine

Status: Active
Source Idea: ideas/open/46_backend_reboot_bir_spine.md
Activated from: fresh activation on 2026-04-13 after closing the x86-specific recovery idea and resetting the route onto a backend-wide BIR-first reboot

## Purpose

Replace the old direct-route backend shape with one truthful shared pipeline:

`LIR -> semantic BIR -> prepare/legalize -> target backend`

## Goal

Make BIR the real backend spine so x86, aarch64, and riscv64 no longer depend
on raw-LIR fallback paths or testcase-shaped recovery hooks.

## Core Rule

Do not reintroduce `try_emit_*`, `try_lower_*`, direct-dispatch ladders,
rendered-text case matchers, or equivalent workaround seams under new names.

## Read First

- `ideas/open/46_backend_reboot_bir_spine.md`
- `src/codegen/lir/ir.hpp`
- `src/backend/backend.cpp`
- `src/backend/bir.hpp`
- `src/backend/bir_printer.cpp`
- `src/backend/bir_validate.cpp`
- `src/backend/lowering/lir_to_bir.hpp`
- `src/backend/lowering/lir_to_bir.cpp`
- `src/backend/lowering/lir_to_bir_module.cpp`
- `src/backend/prepare/prepare.hpp`
- `src/backend/prepare/prepare.cpp`
- `src/backend/prepare/legalize.cpp`

## Current Targets

- `src/backend/lowering/lir_to_bir_memory.cpp`
- `src/backend/lowering/lir_to_bir_types.cpp`
- `tests/backend/case/nested_member_pointer_array.c`
- `tests/backend/case/local_array.c`

## Route Checkpoint

- 2026-04-14 lifecycle checkpoint after five executor commits on the direct
  prepare/BIR explicit-phi harness:
  backlog-item-1 reducible explicit-phi materialization is now sufficiently
  sampled for route purposes, with accepted direct coverage for
  multi-incoming funnels, return-only joins, successor-consumed joins,
  forwarded-successor joins, and conditional-successor joins
- keep backlog item 1 parked at that accepted baseline for now; do not send
  another executor packet that only extends the current reducible-phi harness
  or restarts stale select/phi testcase scouting without naming a fresh
  honest merge seam first
- route-quality re-sequencing is now justified:
  backlog item 2 already has truthful behavior-first proving surfaces on
  `param_slot`, `param_member_array`, and `nested_param_member_array`, so the
  active executor route advances there instead of leaving the whole runbook
  blocked on an unnamed next item-1 seam
- treat `tests/cpp/internal/backend_prepare_phi_materialize_test.cpp` and the
  landed explicit-phi prepare work as regression baseline for backlog item 1,
  not as the current packet source
- 2026-04-14 supervisor validation plus review follow-up kept this runbook on
  the same source idea, but it also proved the previously sampled
  `backend_codegen_route` select/phi inventory is exhausted as a truthful next
  executor surface for backlog item 1
- do not send another packet from stale `*_asm_unsupported`
  select/phi/callee stems, nearby already-green prepared-BIR select routes, or
  observation-only harness churn
- the next honest backlog-item-1 seam is prepare-side explicit phi
  materialization in `src/backend/prepare/legalize.cpp` beyond the currently
  proven reducible / two-incoming path
- semantic-BIR observation may still be used, but only as proof support for a
  real lowering/prepare code move; it is no longer an authorized packet by
  itself
- if the supervisor cannot name one proof source tied to that seam in the same
  packet, stop for another lifecycle checkpoint instead of forcing execution
- 2026-04-14 supervisor scouting widened that exhaustion result from a few
  named stems to the current truthful C route surface as a whole:
  checked-in riscv64 `backend_route_case` select/phi sources now lower on the
  default prepared route to `bir.select`, not phi-slot materialization, while
  extra throwaway statement/goto/loop probes bypass semantic `bir.phi`
  entirely and stay mutable-local instead
- treat further C-route testcase hunting for this seam as blocked route churn;
  the next honest proving surface must come from a direct prepare/BIR-facing
  harness tied to a real `src/backend/prepare/legalize.cpp` code move, or from
  another lifecycle checkpoint if that harness still cannot be named cleanly
- 2026-04-14 lifecycle checkpoint decision:
  keep this runbook active on the same source idea, but treat executor
  dispatch as explicitly blocked until one truthful proving surface can expose
  the prepare-side explicit-phi seam without falling back to stale
  `*_asm_unsupported` inventory, nearby already-green prepared-BIR routes, or
  observation-only harness churn
- until that proving surface exists, supervisor should commit only lifecycle
  state for this checkpoint and stop rather than inventing another bounded
  executor packet from exhausted backlog-item-1 evidence
- 2026-04-14 lifecycle checkpoint after backlog-item-2 signature scouting:
  keep the same source idea and runbook, but park backlog item 2 at the
  accepted non-variadic semantic-signature baseline instead of forcing more
  x86 signature-lane churn
- honest x86 semantic-BIR signature coverage now includes the accepted
  parameter, aggregate `byval`, hidden `sret`, direct function-symbol,
  indirect function-pointer, direct object-address pointer, loaded global
  pointer, and aggregate function-pointer call surfaces already checked into
  `tests/backend/CMakeLists.txt`
- the nearby non-BIR misses found during supervisor scouting are
  `tests/backend/case/nested_member_pointer_array.c`, which belongs to shared
  local pointer-bearing address formation, and the `variadic_*` cases, which
  belong to later ABI/variadic work rather than the active non-variadic
  signature lane
- route-quality re-sequencing is therefore justified again:
  the next honest executor packet advances to backlog item 3 local
  memory/address semantics instead of extending item-2 harness inventory or
  pulling variadic ABI work forward
- do not send another executor packet whose main effect is more backlog-item-2
  semantic-BIR sentinel churn unless a fresh non-ABI signature seam appears

## Non-Goals

- do not rebuild target asm emission before the shared BIR/prepare spine is
  trustworthy
- do not claim c-testsuite recovery from direct route deletion alone
- do not patch individual case families with route-specific detection logic
- do not push target legality checks back into semantic `lir_to_bir`
- do not let packet selection drift into testcase names when the real missing
  unit is a semantic capability family

## Working Model

- semantic BIR must preserve compiler meaning, not LLVM text artifacts
- target legality belongs in `prepare/legalize`, not in `lir_to_bir`
- structural validation may stay in BIR, but type legality should not block
  semantic lowering
- executor packets should be chosen from the ordered capability backlog below,
  not rediscovered from ad hoc testcase hunting
- stale unsupported-route inventory is harness debt, not proof of the next
  missing capability
- backlog item 1 explicit-phi materialization now stays parked as
  non-regression coverage, backlog item 2 signature work stays parked as the
  accepted non-variadic baseline, and executor focus now moves to local
  memory/address formation

## Completed Capability Baseline

These lanes are already landed and should remain green while new families are
added:

- scalar compare / branch / return
- single-parameter `select` lowering from `diamond + phi`
- non-variadic semantic function signatures for scalar params, aggregate
  `byval`, hidden `sret`, direct/indirect pointer calls, and loaded-global
  pointer call/return flows
- local scalar slots, loads, and stores
- local arrays with constant indexing
- direct global calls with minimal typed args
- scalar globals with load and store

## Ordered Capability Backlog

Choose future executor packets from this list in order unless route quality or
review justifies a re-sequencing.

### 1. Generalize CFG merge and `phi`

Goal: stop treating `phi` as a ternary-only special lane.

Primary target:

- `src/backend/lowering/lir_to_bir_cfg.cpp`
- `src/backend/lowering/lir_to_bir.hpp`
- `src/backend/bir.hpp`

Concrete actions:

- use `ref/claudes-c-compiler/src/backend/generation.rs` as the shape
  reference for shared pre-codegen CFG semantics:
  terminators and ordinary value flow are handled in the shared pipeline
  before later target phases
- use `ref/claudes-c-compiler/src/backend/stack_layout/mod.rs`,
  `stack_layout/slot_assignment.rs`, and `stack_layout/README.md` as the
  downstream contract reference:
  later prepare/stack work should consume phi-eliminated or merge-attributed
  values rather than rediscovering CFG semantics inside call lowering
- widen beyond `diamond + phi -> bir.select`
- support ordinary block-merge `phi` forms without testcase-shaped CFG probes
- treat testcase families only as proving surfaces for this lane, never as the
  semantic classification used by lowering or prepare:
  loop-header merges, nested non-diamond joins, successor-forwarded merge
  values, and `break` / `continue`-driven joins must all flow through the same
  shared merge mechanism rather than per-pattern special paths
- define the first honest semantic split:
  canonical diamonds may still lower to `select`, but non-diamond or
  predecessor-attributed merges must remain explicit shared merge semantics
  until later prepare work
- when backlog item 1 is resumed, drive it toward a general CFG-based
  explicit-phi materialization / elimination route in
  `src/backend/prepare/legalize.cpp`, centered on predecessor-edge value
  assignment, critical-edge splitting where needed, and parallel-copy style
  resolution of multi-phi updates instead of syntax-shaped case matching
- checkpoint the next packet on the remaining honest seam:
  generalize explicit phi materialization in `src/backend/prepare/legalize.cpp`
  beyond the currently proven reducible / two-incoming path
- tie that prepare-side code move to one truthful proving source; semantic-BIR
  observation can support proof, but stale `*_asm_unsupported` select stems,
  broader C-route testcase hunting, or harness-only expansion cannot be the
  packet by themselves
- make later lanes consume already-lowered merge semantics:
  call lowering must use shared merged values, not carry its own private
  `phi`/CFG reconstruction rules

Completion check:

- non-trivial merge shapes no longer require raw-LIR fallback
- `phi` handling is explained by CFG semantics, not by one named case family
- accepted widening work demonstrates one shared elimination/materialization
  strategy across distinct CFG shapes rather than separate testcase-specific
  lowering rules
- the first implementation slice changes lowering/BIR files rather than only
  test routing or proving regexes
- the next accepted packet proves a real prepare/lowering code move on explicit
  phi handling, not just cleanup of stale unsupported inventory
- current checkpoint state:
  keep this lane parked at the landed reducible explicit-phi baseline until a
  fresh honest merge seam can be named without route churn

### 2. Harden params and function signatures

Goal: make function entry and return contracts reliable before broader calls or
ABI shaping.

Primary target:

- `src/backend/lowering/lir_to_bir_calling.cpp`
- `src/backend/lowering/lir_to_bir_aggregate.cpp`
- `src/backend/lowering/lir_to_bir.hpp`

Concrete actions:

- do not use the existing `backend_case` `two_arg_*` route stems as
  printed-BIR proof under the current backend contract:
  once lowering succeeds, `src/backend/backend.cpp` legitimately prepares BIR
  and emits target asm for those helpers
- treat helper entry lowering, direct-call argument materialization, and return
  propagation as one semantic signature lane in
  `src/backend/lowering/lir_to_bir_calling.cpp` plus the
  `BirFunctionLowerer` signature helpers declared in
  `src/backend/lowering/lir_to_bir.hpp`, but keep the `two_arg_*` helper
  family as runtime/regression sentinels rather than the primary proof anchor
- use `param_slot`, `param_member_array`, and
  `nested_param_member_array` as the first honest backlog-item-2 proving
  surface because they exercise parameter handling by behavior rather than by
  output-mode choice
- if direct observation of signature-lowering success still needs more than
  runtime behavior, allow one narrow harness packet that can observe semantic
  BIR coverage without regressing supported prepared-BIR emission
- keep semantic signature lowering separate from target ABI legalization
- do not count native-asm output on trivial helpers or frontend-promotion
  expectation mismatches as proof of semantic signature progress

Completion check:

- parameter-slot and simple aggregate-param cases are proven by honest
  backend-case behavior or approved harness observation, not by forcing
  supported prepared-BIR emission to print BIR
- scalar direct helper/runtime sentinels remain consistent with the same
  signature-lowering rule instead of requiring named-case branches
- current checkpoint state:
  keep this lane parked at the accepted non-variadic signature baseline until
  a fresh honest signature seam appears that does not actually belong to local
  address formation or later ABI/variadic work

### 3. Broaden local memory and address formation

Goal: move from scalar locals and constant-index local arrays to general local
address semantics.

Primary target:

- `src/backend/lowering/lir_to_bir_memory.cpp`
- `src/backend/lowering/lir_to_bir_types.cpp`

Design contract:

- local memory lowering must use one shared object/address model:
  scalar locals, arrays, structs, nested aggregates, and pointer-bearing local
  objects should differ by layout and access path, not by entering separate
  lowering families
- testcase families are proving surfaces only:
  `struct` field reads, nested-field paths, array-of-struct cases,
  struct-containing-array cases, address-taken locals, and local pointer
  round-trip helpers must not become the semantic classification used by
  lowering
- address formation should be represented by meaning:
  local base object plus element/field/byte offset path, so later load/store,
  call-byval, and pointer-passing work can consume the same semantic address
  rather than re-deriving local object shape ad hoc

Concrete actions:

- define or tighten one local object/address representation that can describe
  scalar, array, and struct-backed locals with the same base-plus-offset/path
  semantics
- extend `alloca` / local-slot lowering beyond scalar and fixed tiny array
  shapes
- lower broader `gep` and address forms for locals
- handle struct and nested-aggregate local addressing through shared layout and
  offset computation instead of field-shape-specific branches
- support pointer-valued locals and round-tripped local addresses where the
  semantics are still target-neutral
- keep layout computation centralized:
  size/align/field-offset reasoning for local aggregates should not be copied
  into multiple lowering branches keyed off named cases
- make later lanes consume the same local-address semantics:
  loads, stores, aggregate copies, and call argument materialization should all
  use the shared local object/address model rather than inventing their own
  struct/array recovery paths
- reject these route drifts for this lane:
  per-struct-shape lowering branches, separate local-array vs local-struct
  address families, testcase-specific `gep` recognition, or field-name-pattern
  routing that bypasses shared layout semantics

Completion check:

- local memory lowering no longer stops at constant-index toy arrays
- struct, nested-aggregate, and pointer-addressed locals are explained by one
  shared local object/address strategy rather than by separate testcase
  recoveries
- local address formation is reusable by later call/memory lanes without
  re-inferring aggregate layout from testcase shape
- current checkpoint state:
  start this lane from pointer-bearing local aggregate address formation on
  `tests/backend/case/nested_member_pointer_array.c`; keep
  `tests/backend/case/local_array.c` as nearby regression coverage, and do not
  mix variadic ABI work into this packet

### 4. Broaden global data and addressed globals

Goal: make module-level state real beyond scalar globals.

Primary target:

- `src/backend/lowering/lir_to_bir_globals.cpp`
- `src/backend/lowering/lir_to_bir_types.cpp`
- `src/backend/bir.hpp`

Design contract:

- global data lowering should reuse the same semantic address principles as
  local objects wherever possible:
  a global base object plus offset/path should be the primary model for arrays,
  structs, strings, extern objects, and addressed global reads
- testcase families are proving surfaces only:
  string-backed globals, global arrays, extern globals, addressed struct
  globals, and pointer-valued globals must not each grow their own hardcoded
  lowering family
- addressed global semantics should stay distinct from target relocation or ABI
  concerns:
  this lane owns semantic global object/value meaning, not final target address
  materialization policy

Concrete actions:

- define global object/address lowering in terms of shared base-plus-offset/path
  semantics so global arrays, structs, strings, and addressed reads can reuse
  one representation
- lower global arrays and string-backed globals
- support extern global declarations and addressed global reads
- support pointer-valued globals and basic global-address arithmetic only where
  the semantics are still honest BIR
- handle struct-backed globals and nested aggregate global addresses through the
  same centralized layout/offset machinery used for other aggregate addressing,
  not through testcase-specific global field paths
- keep global object semantics aligned with local object semantics where honest:
  later load/store/call lanes should consume addressed globals through the same
  conceptual model instead of branching on storage class
- reject these route drifts for this lane:
  separate lowering families for string globals vs array globals vs struct
  globals, helper-name-based global-address shortcuts, or special-case global
  field materialization that does not generalize beyond one aggregate shape

Completion check:

- addressed global data no longer forces LLVM-text fallback for simple array or
  string-backed reads
- global struct/array/string addressing is explained by one semantic
  object-address model rather than by storage-class-shaped patches
- later consumers can use addressed global values without rediscovering layout
  or inventing global-only aggregate rules

## Deferred Follow-On Ideas

The long-tail backlog beyond the current lowering route now lives in separate
open ideas so this runbook stays focused on the active and near-term semantic
BIR work:

- `ideas/open/47_semantic_call_runtime_boundary.md`
  for richer call lowering, intrinsic/runtime lowering, and semantic
  unsupported-boundary definition
- `ideas/open/48_prepare_pipeline_rebuild.md`
  for making `prepare` the real owner of legality plus rebuilding stack
  layout, liveness, and regalloc as real phases
- `ideas/open/49_prepared_bir_target_ingestion.md`
  for reconnecting x86, aarch64, and riscv64 to prepared BIR only

## Validation Ladder

- `cmake --build build -j2`
- focused backend-route observation using
  `./build/c4cll --codegen asm --target <triple> <case>.c -o <out>`
- escalate to matching internal/c-testsuite subsets only after the BIR/prepare
  lane for that capability is real
