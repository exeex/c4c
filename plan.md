# AArch64 Intrinsic Machine Nodes Runbook

Status: Active
Source Idea: ideas/open/239_aarch64_intrinsic_machine_nodes.md
Activated From: ideas/open/239_aarch64_intrinsic_machine_nodes.md

## Purpose

Finish the remaining AArch64 intrinsic machine-node route now that scalar
`fabs` selection/printing is complete and CRC/vector semantic plus prepared
carrier authority exists.

## Goal

Select and print accepted AArch64 intrinsic machine records only from complete
structured carriers, while unsupported or incomplete intrinsic paths continue
to fail closed with diagnostics.

## Core Rule

Machine support must consume explicit semantic, prepared, and selected AArch64
records. Do not infer support from intrinsic spelling, generic call plans,
archived scratch-register conventions, or final assembly text.

## Read First

- `ideas/open/239_aarch64_intrinsic_machine_nodes.md`
- `ideas/closed/241_aarch64_crc_vector_intrinsic_carriers.md`
- `src/backend/bir/bir.hpp`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prepared_printer.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
- `tests/backend/mir/backend_aarch64_machine_printer_test.cpp`

## Current Scope

- CRC32W, vector load, and vector add machine-node selection and printing from
  the completed carrier route.
- Barrier, cache, pause/hint, and builtin-address intrinsic families only when
  current structured semantic and prepared carrier authority is already
  present.
- Fail-closed diagnostics for unsupported x86-only, missing-feature,
  missing-operand, malformed-vector-shape, incomplete-carrier, and
  non-selected printer paths.
- Lifecycle dependency notes or new source ideas for remaining intrinsic
  families that still lack upstream carrier authority.

## Non-Goals

- Do not redo scalar `fabs` carrier, selection, or printer work except to keep
  it passing while extending the route.
- Do not rebuild archived `x0`/`w9`/`q0`/`q1` scratch-register conventions.
- Do not add AArch64 support for all x86 SSE/AES/CLMUL intrinsics.
- Do not merge this route with inline asm, atomics, ordinary calls, frame
  lowering, binary128 helpers, or generic memory-node work.
- Do not weaken unsupported-path expectations or claim progress through
  expectation rewrites.

## Working Model

- BIR owns intrinsic family, operation, feature, operand, memory, width, lane,
  and result semantics.
- Prepared intrinsic carriers own completeness and register/home authority.
- AArch64 dispatch consumes complete prepared intrinsic carriers into selected
  machine records.
- The machine printer emits only from selected AArch64 records with explicit
  operand/result register authority.
- Missing upstream carrier facts are dependency blockers, not permission to
  recover semantics from names or text.

## Execution Rules

- Keep each step small enough to prove with `cmake --build --preset default`
  plus the supervisor-delegated backend subset.
- Add or preserve fail-closed tests with every new accepted intrinsic family.
- Prefer structured enum/data fields over intrinsic-name matching or fixture
  matching.
- If a remaining family lacks carrier authority, record that in `todo.md` and
  ask for lifecycle split instead of silently expanding this runbook.
- Treat expectation downgrades, fabricated registers, and single-fixture
  shortcuts as route drift.

## Step 1: Inventory Post-Carrier Machine-Node Readiness

Goal: identify which remaining intrinsic families can now be selected and
which still require separate carrier work.

Primary targets:

- `ideas/open/239_aarch64_intrinsic_machine_nodes.md`
- `ideas/closed/241_aarch64_crc_vector_intrinsic_carriers.md`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`

Actions:

- Confirm the completed CRC/vector carrier fields available to AArch64
  dispatch.
- Inspect barrier, cache, pause/hint, and builtin-address inputs for complete
  semantic and prepared carrier authority.
- Record in `todo.md` which families are ready for machine selection and which
  need dependency ideas before selection or printing.
- Keep existing scalar `fabs` selected-machine behavior as the regression
  baseline.

Completion check:

- `todo.md` names the ready families and any carrier-authority blockers.
- No new selected records or printer output are required unless needed to
  preserve fail-closed diagnostics.
- Backend proof passes for the delegated inventory subset.

## Step 2: Define Selected Intrinsic Machine Records

Goal: add selected AArch64 record shapes for the ready intrinsic families
without printing assembly text yet.

Primary targets:

- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `tests/backend/mir/backend_aarch64_target_instruction_records_test.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`

Actions:

- Add target record fields for intrinsic family, operation, feature contract,
  operand/result registers, vector shape, memory access, and immediate facts
  needed by ready families.
- Preserve scalar `fabs` record behavior.
- Keep unsupported or incomplete records unprintable and diagnosable.

Completion check:

- Record-level tests prove selected record fields preserve the carrier
  authority needed by dispatch and printer steps.
- No assembly text is emitted for CRC/vector or other newly ready families in
  this step.

## Step 3: Select CRC And Vector Intrinsic Records

Goal: consume complete prepared CRC/vector carriers into selected AArch64
machine records.

Primary targets:

- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`

Actions:

- Select `crc32w` only when the CRC feature, operation, operands, result, and
  prepared homes are complete.
- Select vector load and vector add only when vector shape, memory or operand
  facts, feature facts, operands, result, and prepared homes are complete.
- Preserve fail-closed diagnostics for missing homes, malformed vector shape,
  unsupported x86-only intrinsics, and incomplete carriers.
- Do not add printer spelling in this step.

Completion check:

- Dispatch tests cover one complete representative for CRC32W, vector load,
  and vector add.
- Nearby malformed or unsupported cases remain diagnostic-only.
- Scalar `fabs` dispatch tests still pass.

## Step 4: Print Structured CRC And Vector Intrinsic Records

Goal: emit AArch64 assembly text for the selected CRC/vector records only.

Primary targets:

- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `tests/backend/mir/backend_aarch64_machine_printer_test.cpp`

Actions:

- Print CRC32W, vector load, and vector add from selected records with explicit
  register and vector-shape authority.
- Reject fabricated intrinsic-shaped records that lack selected-machine
  authority or required fields.
- Keep printer output separate from prepared-printer diagnostics.

Completion check:

- Printer tests prove final spelling comes from selected records and preserves
  operand/result contracts.
- Non-selected, incomplete, or unsupported intrinsic-shaped records do not
  print machine instruction lines.

## Step 5: Handle Remaining Ready Families Or Split Dependencies

Goal: complete any barrier/cache/pause/hint/builtin-address machine-node work
that already has structured carrier authority, and split any remaining carrier
gaps into separate source ideas.

Primary targets:

- `src/backend/bir/`
- `src/backend/prealloc/`
- `src/backend/mir/aarch64/codegen/`
- `tests/backend/bir/`
- `tests/backend/mir/`
- `ideas/open/`

Actions:

- For each remaining family with complete carrier authority, add selected
  records, dispatch tests, printer support, and fail-closed diagnostics using
  the same record-first pattern as CRC/vector.
- For each remaining family without complete carrier authority, record the
  blocker in `todo.md` and request lifecycle split into a dependency idea.
- Do not add name-only selected records to make the family appear supported.

Completion check:

- Ready families have dispatch and printer proof.
- Blocked families are explicitly recorded as dependency work rather than
  hidden behind text emission or weakened tests.

## Step 6: Prove Route And Decide Closure

Goal: prove the intrinsic machine-node route and decide whether source idea
239 is complete, blocked, or needs a follow-on runbook.

Primary targets:

- `tests/backend/bir/`
- `tests/backend/mir/`
- `ideas/open/239_aarch64_intrinsic_machine_nodes.md`

Actions:

- Run the supervisor-delegated proof subset for BIR carrier boundaries,
  prepared-printer proof, AArch64 dispatch, and AArch64 machine printer tests.
- Escalate to `ctest --test-dir build -j --output-on-failure -R '^backend_'`
  before treating the route as milestone-complete.
- Confirm binary128 helpers remain delegated and unsupported x86-only
  intrinsics remain rejected, trapped, or diagnosed by policy.
- Request lifecycle closure, repair, or dependency split based on the actual
  remaining source scope.

Completion check:

- Backend proof passes with no regressions.
- `todo.md` records supported families, explicit unsupported families, and any
  separate dependency ideas.
- Plan owner can determine whether idea 239 is complete or still open.
