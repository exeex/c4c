# Prepared Global Data Authority Runbook

Status: Active
Source Idea: ideas/open/608_prepared_global_data_authority.md
Activated from: ideas/open/608_prepared_global_data_authority.md

## Purpose

Complete prepared/global authority for selected object data, prepared global
memory facts, and direct global-symbol base-plus-offset addressing before any
RV64 global emission work consumes those facts.

## Goal

Move multiple prepared/global authority rows past the current prepared stop
while keeping RV64/global consumer rows fail-closed for the later `609` idea.

## Core Rule

Publish prepared authority facts first. Do not repair this idea by emitting
RV64 globals, widening RV64 access support, changing expectations, or matching
representative testcase names.

## Read First

- ideas/open/608_prepared_global_data_authority.md
- docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md
- docs/rv64_gcc_torture_1000_pass_recovery/dependency_order_to_1000.md
- docs/rv64_gcc_torture_post_contract/infrastructure_bucket_evidence.md
- src/backend/prealloc/object_data.cpp
- src/backend/prealloc/object_data.hpp
- src/backend/prealloc/prepared_contract_verifier.cpp
- src/backend/prealloc/prepared_contract_verifier.hpp
- src/backend/prealloc/addressing.hpp
- src/backend/prealloc/stack_layout/coordinator.cpp

## Current Targets

- Selected global object-data contract rows: `17`, representative
  `src/20010924-1.c`.
- Prepared global memory facts rows: `12`, representative `src/strlen-7.c`.
- Direct global-symbol base-plus-offset rows: `11`, representative
  `src/pr79737-2.c`.
- Handoff facts that later RV64/global consumer work in `609` can rely on.

## Non-Goals

- RV64 global symbol emission.
- RV64 global access-width lowering or widening.
- BIR global initializer bootstrap work.
- Local memory, ABI, runtime/link, timeout, allowlist, accounting, or
  unsupported-marker policy changes.
- Expectation rewrites or testcase-specific matching.

## Working Model

The global-data bucket is intentionally split across two owners. This runbook
owns the `40` prepared/global authority rows: selected object-data contract,
prepared global memory facts, and direct global-symbol base-plus-offset
authority. RV64 diagnostics such as `cannot emit prepared global symbol` or
access-width support remain target-consumer failures for `609` unless the row
first needs missing prepared facts from this plan.

Prepared object-data and memory authority should be expressed in existing
prepared facts, contract verification, and addressing/publication helpers.
Target code may be inspected for diagnostics, but it should not become the
place where missing prepared facts are reconstructed.

## Execution Rules

- Start every code packet from current failing diagnostics and nearby same
  family rows, not from one representative testcase alone.
- Prefer semantic prepared/global facts over special cases for individual C
  torture files.
- Preserve fail-closed behavior when object data, symbol identity, extent,
  alignment, initializer bytes, zero-fill, relocation, or base-plus-offset
  evidence is missing or contradictory.
- Keep `todo.md` as the mutable executor state. Do not rewrite this runbook
  for routine packet progress.
- For code-changing steps, prove with a fresh build or compile proof plus the
  supervisor-selected narrow RV64 gcc-torture subset.
- Escalate to broader validation when one packet touches shared prepared
  contract or addressing helpers used outside global data.

## Ordered Steps

### Step 1: Inventory prepared/global authority blockers

Goal: identify which current rows are missing prepared authority facts and
which rows are already prepared but blocked only by later RV64 consumption.

Primary targets:
- `build/agent_state/rv64_gcc_c_torture_backend_failed.txt`
- `build/rv64_gcc_c_torture_backend/<case-id>/case.log`
- diagnostics from `src/backend/prealloc/prepared_contract_verifier.cpp`
- diagnostics in `src/backend/mir/riscv/codegen/object_emission.cpp`

Actions:
- Inspect the current selected object-data, prepared global memory, and direct
  global-symbol base-plus-offset diagnostics.
- Pick a narrow representative set that covers at least two prepared authority
  families when possible.
- Record in `todo.md` which rows are authority-owned and which must stay
  RV64/global consumer failures for `609`.
- Identify the existing prepared facts and helpers that should own each
  missing fact before implementation.

Completion check:
- `todo.md` names the exact Step 1 evidence set, candidate rows, and first
  implementation target without broadening into RV64 emission.

### Step 2: Publish prepared global memory facts

Goal: make supported global load/store memory accesses carry prepared facts
that RV64 can consume later without reconstructing address provenance.

Route status: parked after evidence-gated executor review. Captured predicate
inputs for `src/strlen-7.c` and `src/20000703-1.c` showed the first visible
missing field was `layout_authority=unknown`; a narrow prepared producer
experiment could publish `ByteStorageAggregate` in prepared dumps, but the
exact allowlist proof stayed at `0/7` with unchanged
`requires supported prepared global memory facts` diagnostics. Do not repeat
that helper-only publication route unless a future 608-owned packet can name a
different prepared fact and prove diagnostic movement without touching RV64
consumer/emission code.

Primary targets:
- `src/backend/prealloc/addressing.hpp`
- `src/backend/prealloc/stack_layout/coordinator.cpp`
- existing prepared global load/store lookup and publication helpers.

Actions:
- Before editing code, capture the exact `PreparedAddress` / `PreparedMemoryAccess`
  predicate inputs that make
  `prepared_global_symbol_memory_has_publication_authority()` fail for at
  least two supported prepared global memory rows.
- Trace global load/store lanes from BIR values through prepared address and
  memory-access facts.
- Add or repair authority publication for supported global-symbol memory
  accesses only when the captured evidence shows a missing prepared fact within
  this plan's ownership: symbol identity, base-plus-offset eligibility, offset,
  size, alignment, object extent, requested range, range verdict, or layout
  authority.
- Keep access-width legality and final instruction emission out of scope.
- Preserve fail-closed diagnostics for ambiguous symbols, missing identity,
  unsupported widths, missing initializer/layout facts, and policy-sensitive
  cases.
- If the predicate inputs already show complete prepared authority but the row
  still stops at an RV64 object-route diagnostic, stop and return the evidence
  for lifecycle routing instead of editing RV64 consumer/emission code.
- Reject helper-only publication changes that leave the same rows at the exact
  `requires supported prepared global memory facts` diagnostic.

Completion check:
- This step remains blocked unless at least one supported prepared global
  memory-facts row moves past the exact `requires supported prepared global
  memory facts` stop for a semantic prepared-authority reason. If the row
  remains at the same object-route diagnostic after prepared facts are
  complete, preserve the evidence as a handoff note and keep the RV64 consumer
  work for `609`.

### Step 3: Complete direct global-symbol base-plus-offset authority

Goal: publish direct global-symbol base-plus-offset authority for rows where
symbol identity and byte offset are semantically known.

Route status: parked after evidence-gated executor review. Captured prepared
inputs for `src/pr79737-2.c` and neighboring `src/pr82387.c` showed direct
global-symbol accesses already had constant offsets, nonzero size/alignment,
`base_plus_offset=yes`, and proven ranges; the first visible missing field was
`layout_authority=unknown`. A narrow prepared producer experiment could publish
`ByteStorageAggregate` for the aggregate/bitfield accesses, but the exact
allowlist proof stayed at `0/5` with unchanged `requires prepared direct
global-symbol base-plus-offset memory addressing` diagnostics. Do not repeat
that helper-only publication route unless a future 608-owned packet can name a
different prepared fact and prove diagnostic movement without touching RV64
consumer/emission code.

Primary targets:
- `src/backend/prealloc/addressing.hpp`
- `src/backend/prealloc/stack_layout/coordinator.cpp`
- selected-address and address-materialization helpers.

Actions:
- Inspect how direct global-symbol addresses are resolved and represented in
  prepared address facts.
- Repair the prepared authority path for direct symbol plus constant offset
  where the global, offset, range, and use are all known.
- Do not treat relocation/materialization facts as pointer freshness authority.
- Keep GOT/TLS, target relocation emission, and RV64 materialization policy out
  of this step unless they already exist as prepared facts that must be
  preserved.
- If captured predicate inputs already show complete direct global-symbol
  base-plus-offset authority but the row still stops at an RV64 object-route
  diagnostic, stop and return the evidence for lifecycle routing instead of
  editing RV64 consumer/emission code.
- Reject helper-only layout-authority publication changes that leave the same
  rows at the exact `requires prepared direct global-symbol base-plus-offset
  memory addressing` diagnostic.

Completion check:
- This step remains blocked unless at least one direct global-symbol
  base-plus-offset row moves past the exact direct base-plus-offset diagnostic
  for a semantic prepared-authority reason. If the row remains at the same
  object-route diagnostic after prepared facts are complete, preserve the
  evidence as a handoff note and keep the RV64 consumer work for `609`.

### Step 4: Complete selected global object-data authority

Goal: make selected global object-data rows publish coherent authority for
label, identity, extent, alignment, emitted bytes, zero-fill, relocation, and
unsupported-marker state.

Route status: accepted relocation-only pointer object-data progress. The
`src/921110-1.c` row moved past the prepared selected object-data contract stop
to the RV64 relocation-record consumer diagnostic after one-slot pointer object
data began publishing relocation-required/relocation-present authority.
Neighboring mixed or aggregate rows remain fail-closed at the prepared contract
stop unless prepared emitted bytes plus relocation slots can be represented
safely. Do not route the remaining relocation-record consumer work into this
plan.

Primary targets:
- `src/backend/prealloc/object_data.cpp`
- `src/backend/prealloc/object_data.hpp`
- `src/backend/prealloc/prepared_contract_verifier.cpp`
- `src/backend/prealloc/prepared_contract_verifier.hpp`

Actions:
- Audit how `PreparedGlobalObjectData` is populated from BIR globals.
- Repair missing or contradictory prepared object-data facts only where the BIR
  initializer and global layout evidence supports them.
- Preserve explicit unsupported or invalid-prepared diagnostics when the
  initializer facts are absent or out of scope.
- Avoid moving byte emission into RV64 as a substitute for prepared authority.

Completion check:
- At least one selected global object-data family row advances past the
  prepared object-data contract stop, and nearby rows either advance for the
  same semantic reason or retain a precise fail-closed diagnostic.

### Step 5: Prove prepared authority handoff and preserve the split

Goal: demonstrate that this plan improved prepared/global authority without
absorbing the later RV64/global consumer idea.

Primary targets:
- selected global object-data rows
- selected prepared global memory facts rows
- selected direct global-symbol base-plus-offset rows
- neighboring RV64/global consumer rows for split preservation

Actions:
- Run the supervisor-selected narrow RV64 gcc-torture proof after each code
  packet.
- Re-run the selected object-data proof around the Step 4 relocation-only row
  and record the diagnostic movement from prepared contract stop to RV64
  relocation-record consumer stop.
- Include the parked Step 2 and Step 3 evidence showing that helper-only
  `ByteStorageAggregate` publication did not move the exact prepared-memory or
  direct base-plus-offset diagnostics.
- Confirm proof covers more than one prepared authority family where possible.
- Confirm global symbol emission and global access-width rows remain classified
  as RV64/global consumer work unless missing prepared authority was the real
  first owner.
- Update `todo.md` with proof commands, results, and remaining handoff notes.

Completion check:
- Prepared/global authority movement and parked evidence are recorded in
  `todo.md`, the remaining prepared stops are named precisely, and `609`
  remains the follow-up owner for RV64 global emission, relocation-record
  consumption, or access-width consumption.
