# AArch64 Fallthrough Fixed-Offset Local Load Store Emission Runbook

Status: Active
Source Idea: ideas/open/341_aarch64_fallthrough_fixed_offset_local_load_store_emission.md
Activated From: ideas/open/340_aarch64_scalar_cast_stack_homed_register_source_publication.md residual split

## Purpose

Repair the generated-code residual where fallthrough BIR blocks contain
fixed-offset local loads and stores, but emitted AArch64 only advances pointer
locals and omits the data-copy instructions.

## Goal

Make AArch64 emission preserve fixed-offset local load/store operations in
switch/Duff-device fallthrough blocks.

## Core Rule

Fix the fallthrough fixed-offset local load/store emission path generally. Do
not special-case `00143`, Duff-device labels, block numbers, pointer-local
names, source lines, or emitted instruction strings.

## Read First

- `ideas/open/341_aarch64_fallthrough_fixed_offset_local_load_store_emission.md`
- `ideas/open/340_aarch64_scalar_cast_stack_homed_register_source_publication.md`
  lifecycle note for split evidence
- `todo.md` from the completed idea 340 packet, if available in git history,
  for temporary probe paths and focused proof command context
- Current residual evidence:
  - the old scalar-cast structured-source diagnostic is absent
  - `00143` assembles, links, runs, and fails as `[RUNTIME_NONZERO]`
  - prepared BIR contains fallthrough loads from `a[1]` through `a[7]` and
    stores to `b[1]` through `b[7]`
  - generated AArch64 fallthrough labels only update the `from`/`to` pointer
    locals and omit the expected `ldrh`/`strh` data-copy operations

## Current Targets

- Primary representative:
  - `c_testsuite_aarch64_backend_src_00143_c`
- Useful narrow probes:
  - `ctest --test-dir build -j --output-on-failure -R 'c_testsuite_aarch64_backend_src_00143_c'`
  - `./build/c4cll --dump-prepared-bir --target aarch64-linux-gnu --mir-focus-function main tests/c/external/c-testsuite/src/00143.c`
  - `./build/c4cll --codegen asm --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00143.c -o /tmp/c4c_00143.s`
- Focused backend coverage should exercise fallthrough blocks containing
  fixed-offset local load/store operations and prove the corresponding AArch64
  data movement is emitted.

## Non-Goals

- Do not edit expectation, unsupported, runner, timeout, CTest registration, or
  proof-log policy.
- Do not reopen scalar-cast source-publication work unless the old structured
  register-source diagnostic returns.
- Do not broaden into frame layout, aggregate, variadic, compare, parser,
  semantic HIR, or frontend rewrites without fresh first-bad-fact evidence.

## Working Model

Prepared BIR still contains the required fixed-offset local loads and stores in
the fallthrough blocks. The failure is likely between prepared BIR and emitted
AArch64: lowering, selected-node construction, instruction scheduling, local
access materialization, or printer emission drops those data-copy operations.

## Execution Rules

- Keep routine localization and proof notes in `todo.md`.
- Add focused backend coverage before or with the repair when the fallthrough
  load/store behavior can be expressed without relying only on `00143`.
- Preserve the scalar-cast boundary from idea 340 unless fresh evidence moves
  the first bad fact back there.
- Reclassify any remaining `00143` residual by prepared-state,
  generated-code, assembler/linker output, or runtime evidence before claiming
  completion.

## Steps

### Step 1: Localize Fallthrough Load Store Drop Boundary

Goal: identify where fixed-offset local load/store operations in fallthrough
blocks stop being visible to AArch64 instruction emission.

Primary target: prepared BIR blocks that load from `a[1]` through `a[7]` and
store to `b[1]` through `b[7]` but produce fallthrough assembly without the
matching `ldrh`/`strh`.

Actions:

- Reproduce the `00143` runtime residual and regenerate prepared BIR plus
  AArch64 assembly probes.
- Trace one omitted fallthrough load/store pair from prepared BIR through
  lowering, selected-node construction, scheduling, and printer emission.
- Identify whether the drop is caused by BIR traversal, selected instruction
  construction, local access materialization, branch/fallthrough handling, or
  printer admission.
- Record the concrete first bad owner and proposed repair boundary in
  `todo.md`.

Completion check:

- `todo.md` names the concrete lowering, selection, scheduling, local-access,
  or printer-emission boundary where the fixed-offset local load/store is
  lost.

### Step 2: Repair Fallthrough Fixed-Offset Local Load Store Emission

Goal: make emitted AArch64 preserve the prepared fixed-offset local load/store
operations in fallthrough blocks.

Primary target: the boundary localized in Step 1.

Actions:

- Implement the semantic repair without matching `00143`, label names, block
  numbers, local names, source lines, or instruction strings.
- Add or update focused backend coverage for fallthrough fixed-offset local
  load/store emission.
- Preserve existing scalar-cast source publication behavior and unrelated local
  storage/writeback behavior.

Completion check:

- Focused backend coverage proves fallthrough fixed-offset local loads and
  stores survive to emitted AArch64 data-movement instructions.

### Step 3: Prove Representative And Reclassify Residuals

Goal: verify the missing data-copy emission is repaired and classify any new
first bad fact.

Primary target: `c_testsuite_aarch64_backend_src_00143_c`.

Actions:

- Run the supervisor-delegated focused proof command.
- Confirm generated AArch64 for the fallthrough copy path contains the expected
  load/store data movement.
- If `00143` still fails, classify the new first bad fact from prepared BIR,
  generated assembly, assembler/linker output, or runtime output.
- Update `todo.md` with proof results and any residual owner recommendation.

Completion check:

- The focused proof either passes `00143` or records a new first bad fact that
  is outside fallthrough fixed-offset local load/store emission.
