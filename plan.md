# AArch64 Calls Shard Redistribution Runbook

Status: Active
Source Idea: ideas/open/252_aarch64_calls_markdown_shard_implementation_redistribution.md

## Purpose

Reconcile the legacy `calls.md` shard into ordinary compiled AArch64 calls
owners without expanding call or ABI semantics or leaving call-family behavior
stranded in broad codegen files.

## Goal

Make `src/backend/mir/aarch64/codegen/calls.cpp` and
`src/backend/mir/aarch64/codegen/calls.hpp` the real home for valid
call-family construction, lowering, helper, and spelling behavior described by
`src/backend/mir/aarch64/codegen/calls.md`.

## Core Rule

This route is ownership redistribution, not a feature expansion. Preserve
existing call behavior while moving call-family bodies out of broad owners and
delete `calls.md` only after its durable content is reconciled into compiled
owners.

## Read First

- `ideas/open/252_aarch64_calls_markdown_shard_implementation_redistribution.md`
- `src/backend/mir/aarch64/codegen/calls.md`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.hpp`
- `src/backend/mir/AAPCS64_CALL_RETURN_FRAME_CONTRACT.md`
- `src/backend/mir/ALLOCATION_CONTRACT.md`

## Current Scope

- Create `calls.cpp` and `calls.hpp` as the compiled owner for valid calls
  shard behavior.
- Move call-specific selected-node construction, lowering, helper-call
  printing, and spelling behavior from broad owners into the calls owner.
- Keep `instruction.cpp` focused on family-neutral instruction core.
- Keep `dispatch.cpp` as routing into the calls owner rather than a place that
  owns call-family bodies.
- Keep `machine_printer.cpp` from retaining call-specific spelling bodies when
  those can be owned through calls shard helpers.
- Preserve existing direct call, indirect call, argument placement,
  memory-return, clobber, ABI-binding, and result behavior through prepared
  structured carriers.

## Non-Goals

- Do not redistribute any other AArch64 codegen markdown shard.
- Do not redesign `instruction.hpp` or the broad instruction hierarchy solely
  for this shard.
- Do not expand call, ABI, F128, i128, aggregate, variadic, indirect-call, or
  memory-return semantics beyond behavior-preserving relocation.
- Do not reselect ABI classification locally from the markdown shard; consume
  prepared call plans and the AAPCS64 contract as authority.
- Do not downgrade tests, weaken expectations, or mark supported call paths
  unsupported to make movement appear complete.
- Do not recreate a centralized record pile under a new abstraction name.

## Working Model

- `calls.md` is the durable legacy surface inventory and risk list for this
  redistribution.
- Current call ownership is defined by prepared call plans,
  `module::CallRecord`, `module::MoveRecord`, `module::AbiBindingRecord`,
  allocation-result records, `ALLOCATION_CONTRACT.md`, and
  `AAPCS64_CALL_RETURN_FRAME_CONTRACT.md`.
- Calls ownership includes call-family machine-node construction, call
  lowering helpers, direct and indirect call spelling, helper-call spelling,
  call result storage, and call-specific routing when those behaviors already
  exist in structured compiled code.
- Broad files may keep family-neutral data structures, dispatch switches, or
  printer primitives, but should route call behavior to call-owned helpers.
- Missing prepared facts for F128, outgoing stack snapshots, indirect callee
  materialization, call-time alignment, or helper-call resources are carrier
  gaps, not permission for local classification or assembly-text recovery.

## Execution Rules

- Keep each implementation step semantic and ownership-oriented; reject
  testcase-shaped matching or named-case shortcuts.
- Move one coherent call-family surface at a time and keep build proof fresh
  after code movement.
- Prefer existing local helper patterns and namespace conventions in nearby
  shard owners such as `alu.*`, `comparison.*`, `returns.*`, and `operands.*`
  over introducing a new ownership protocol.
- Do not let `dispatch.cpp`, `instruction.cpp`, or `machine_printer.cpp` grow
  new call bodies while claiming redistribution progress.
- Record blockers in `todo.md` when a broad owner cannot be reduced without a
  larger shared-interface or carrier decision; do not rewrite the source idea
  for routine execution findings.
- For code-changing steps, run at least a fresh build or compile proof plus the
  supervisor-selected focused backend subset. Escalate to broader regression
  guard before closure or when touched surfaces cross backend contracts.

## Ordered Steps

### Step 1: Inspect Calls Ownership Surfaces

Goal: identify the call behavior still owned by broad files and map it to
concrete `calls.cpp`/`calls.hpp` extraction targets.

Primary targets:

- `src/backend/mir/aarch64/codegen/calls.md`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.hpp`

Actions:

- Inventory call-specific selected-node construction, lowering helpers,
  helper-call handling, result storage, direct/indirect call spelling, and
  call-family machine-node construction outside a calls owner.
- Separate family-neutral instruction, dispatch, and printing surfaces from
  call-family body ownership.
- Compare existing compiled call ownership against every durable behavior,
  hidden assumption, known failure risk, and rebuild-guidance item documented
  in `calls.md`.
- Identify the first narrow extraction packet and supervisor-selected proof
  command without editing implementation files in this step.

Completion check:

- `todo.md` records concrete call extraction targets, any family-neutral
  boundaries that must remain outside the calls owner, and the narrow proof
  command selected by the supervisor.

### Step 2: Create Calls Owner And Move Construction Routes

Goal: establish `calls.cpp` and `calls.hpp` as compiled call-family owners and
relocate call-specific construction or route bodies from broad owners.

Primary targets:

- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`

Actions:

- Add `calls.cpp` and `calls.hpp` following nearby shard-owner conventions.
- Move call-specific selected-node construction and machine-node helpers to
  call-owned functions.
- Leave `instruction.cpp` with family-neutral instruction core and shared data
  only.
- Leave `dispatch.cpp` as a route into call-owned helpers, not as the owner of
  call body logic.
- Preserve prepared `CallRecord`, `MoveRecord`, `AbiBindingRecord`, preserved
  value, clobber, memory-return, and indirect-call facts.

Completion check:

- A fresh build or delegated compile proof passes.
- `instruction.cpp` and `dispatch.cpp` no longer own the moved call body logic.
- Existing call behavior remains covered by the focused backend proof selected
  by the supervisor.

### Step 3: Move Call Lowering And ABI-Binding Helpers

Goal: relocate call-family lowering helpers while keeping ABI authority in
prepared shared carriers.

Primary targets:

- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/AAPCS64_CALL_RETURN_FRAME_CONTRACT.md`
- `src/backend/mir/ALLOCATION_CONTRACT.md`

Actions:

- Move existing call-specific lowering helpers into the calls owner.
- Preserve register-argument staging through `BeforeCall` moves and
  ABI-binding records so source values survive movement into ABI resources.
- Preserve result handling, memory-return `x8` ownership, direct/indirect call
  materialization, and call clobber behavior only from existing structured
  facts.
- Treat absent outgoing stack, F128, i128, aggregate-splitting, variadic, or
  helper-call facts as carrier gaps to record in `todo.md`, not as local
  classification work.

Completion check:

- A fresh build or delegated compile proof passes.
- Call lowering helpers live in the calls owner where valid.
- No local ABI reclassification or assembly-text recovery was introduced.

### Step 4: Move Call Spelling And Printer-Side Bodies

Goal: route call-specific machine spelling through call-owned helpers where
that ownership is valid.

Primary targets:

- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.hpp`

Actions:

- Identify direct `bl`, indirect `blr`, helper-call, relocation, and
  call-result spelling bodies in the printer.
- Move call-specific spelling helpers into the calls owner when they do not
  belong to a shared machine-printer primitive or table.
- Preserve direct and indirect call spelling from structured machine
  instruction data.
- Keep final assembly spelling derived from structured records, not testcase
  text.

Completion check:

- A fresh build or delegated compile proof passes.
- `machine_printer.cpp` routes call-specific spelling to call-owned helpers or
  keeps only justified family-neutral printer code.
- Focused backend proof shows printed call behavior is preserved.

### Step 5: Reconcile `calls.md` And Close Shard Ownership

Goal: remove the markdown shard only after its durable behavior and risks are
represented in compiled calls owners or in active execution notes.

Primary targets:

- `src/backend/mir/aarch64/codegen/calls.md`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- `todo.md`

Actions:

- Re-check each `calls.md` entry point, ABI configuration note, stack argument
  note, register argument note, call emission and cleanup note, result
  handling note, dependency, hidden assumption, known failure risk, and
  rebuild-guidance item against compiled owners.
- Keep any remaining execution-only blocker in `todo.md`; do not delete
  `calls.md` while durable shard content is still unreconciled.
- Delete `src/backend/mir/aarch64/codegen/calls.md` only when the calls
  compiled owners hold the valid durable content.
- Avoid carrying long prose from `calls.md` into code comments unless it is
  needed to explain a non-obvious invariant.

Completion check:

- `src/backend/mir/aarch64/codegen/calls.md` is deleted.
- `calls.cpp` and `calls.hpp` own the valid calls shard behavior.
- No broad owner retains call-family bulk implementation without a recorded
  family-neutral justification.

### Step 6: Prove Behavior Preservation And Ownership Boundaries

Goal: validate that redistribution preserved call behavior and did not broaden
the slice beyond the source idea.

Primary targets:

- Focused AArch64 backend tests selected by the supervisor.
- Build or compile proof for touched backend code.
- Regression guard scope selected by the supervisor before lifecycle closure.

Actions:

- Run the supervisor-selected focused backend proof covering call lowering and
  printing behavior.
- Include nearby coverage for direct calls, indirect calls, argument movement,
  result handling, helper-call spelling, memory-return behavior, call clobbers,
  and preserved values when such coverage exists or is added as part of
  behavior preservation.
- Escalate to regression guard before closure or if multiple broad backend
  owners were changed.
- Record proof commands and outcomes in `todo.md`.

Completion check:

- Focused backend proof passes.
- Broader regression guard passes when required by the supervisor.
- The final diff contains calls ownership redistribution and
  behavior-preserving proof, without unrelated feature expansion or expectation
  downgrades.
