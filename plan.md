# AArch64 ALU Shard Redistribution Runbook

Status: Active
Source Idea: ideas/open/251_aarch64_alu_markdown_shard_implementation_redistribution.md

## Purpose

Reconcile the legacy `alu.md` shard into ordinary compiled AArch64 ALU owners
without expanding ALU semantics or leaving family-specific behavior stranded in
broad codegen files.

## Goal

Make `src/backend/mir/aarch64/codegen/alu.cpp` and
`src/backend/mir/aarch64/codegen/alu.hpp` the real home for valid ALU-family
construction, lowering, helper, and spelling behavior described by
`src/backend/mir/aarch64/codegen/alu.md`.

## Core Rule

This route is ownership redistribution, not a feature expansion. Preserve
existing ALU behavior while moving ALU-family bodies out of broad owners and
delete `alu.md` only after its durable content is reconciled into compiled
owners.

## Read First

- `ideas/open/251_aarch64_alu_markdown_shard_implementation_redistribution.md`
- `src/backend/mir/aarch64/codegen/alu.md`
- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/alu.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`

## Current Scope

- Move ALU-specific construction, lowering, and helpers into `alu.cpp` and
  `alu.hpp`.
- Keep `instruction.cpp` focused on family-neutral instruction core.
- Keep `dispatch.cpp` as routing into the ALU owner rather than a body owner.
- Keep ALU-specific printer spelling in ALU-owned helpers where that ownership
  is valid.
- Preserve unary integer helpers, bit-count helpers, byte-swap helpers, binary
  lowering, unsigned power-of-two division/remainder reductions, register-direct
  fast paths, accumulator fallback behavior, 32-bit extension behavior, and
  i128 copy behavior.

## Non-Goals

- Do not redistribute any other AArch64 codegen markdown shard.
- Do not redesign `instruction.hpp` or broad instruction hierarchy solely for
  this shard.
- Do not add new ALU semantics beyond behavior-preserving relocation.
- Do not downgrade tests, weaken expectations, or mark supported ALU paths
  unsupported to make movement appear complete.
- Do not recreate a centralized record pile under a new abstraction name.

## Working Model

- `alu.md` is the durable legacy surface inventory and risk list for this
  redistribution.
- ALU ownership includes ALU-family selected-node construction, lowering helper
  bodies, target operation helpers, and ALU-specific spelling helpers when
  they can be owned through the ALU shard.
- Broad files may keep family-neutral data structures, dispatch switches, or
  printer tables, but should route ALU behavior to ALU-owned helpers.
- Behavior preservation includes signed 32-bit `sxtw` handling, unsigned
  zero-extension, right-operand destination-register conflict handling, narrow
  byte-swap shape, `F32` floating negation zero-extension, and two-register i128
  copy.

## Execution Rules

- Keep each implementation step semantic and ownership-oriented; reject
  testcase-shaped matching or named-case shortcuts.
- Move one coherent ALU family at a time and keep build proof fresh after code
  movement.
- Prefer existing local helper patterns and namespace conventions in
  `alu.cpp`, `alu.hpp`, `comparison.*`, and `returns.*` over introducing a new
  ownership protocol.
- Do not let `dispatch.cpp` or `machine_printer.cpp` grow new ALU bodies while
  claiming redistribution progress.
- Record blockers in `todo.md` when a broad owner cannot be reduced without a
  larger shared-interface decision; do not rewrite the source idea for routine
  execution findings.
- For code-changing steps, run at least a fresh build or compile proof plus the
  supervisor-selected focused backend subset. Escalate to broader regression
  guard before closure or when touched surfaces cross backend contracts.

## Ordered Steps

### Step 1: Inspect ALU Ownership Surfaces

Goal: identify the ALU behavior still owned by broad files and map it to
concrete `alu.cpp`/`alu.hpp` extraction targets.

Primary targets:

- `src/backend/mir/aarch64/codegen/alu.md`
- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/alu.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`

Actions:

- Inventory ALU-specific selected-node construction, lowering helpers,
  operation classifiers, and spelling helpers outside `alu.cpp`/`alu.hpp`.
- Separate family-neutral instruction, dispatch, and printing surfaces from
  ALU-family body ownership.
- Compare existing compiled ALU ownership against every durable behavior and
  hidden assumption documented in `alu.md`.
- Identify the first narrow extraction packet and supervisor-selected proof
  command without editing implementation files in this step.

Completion check:

- `todo.md` records concrete ALU extraction targets, any family-neutral
  boundaries that must remain outside the ALU owner, and the narrow proof
  command selected by the supervisor.

### Step 2: Move ALU Construction And Lowering Bodies

Goal: relocate ALU-family construction and lowering implementation from broad
owners into `alu.cpp` and `alu.hpp`.

Primary targets:

- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/alu.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`

Actions:

- Move ALU-specific selected-node construction and lowering helper bodies to
  ALU-owned functions.
- Leave `instruction.cpp` with family-neutral instruction core and shared data
  only.
- Leave `dispatch.cpp` as a route into ALU-owned helpers, not as the owner of
  ALU body logic.
- Preserve unsigned power-of-two division/remainder reductions, register-direct
  fast paths, accumulator fallback behavior, right-operand destination-register
  conflict handling, and i128 copy behavior.

Completion check:

- A fresh build or delegated compile proof passes.
- `instruction.cpp` and `dispatch.cpp` no longer own the moved ALU body logic.
- Existing ALU behavior remains covered by the focused backend proof selected
  by the supervisor.

### Step 3: Move ALU Spelling And Printer-Side Bodies

Goal: route ALU-specific machine spelling through ALU-owned helpers where that
ownership is valid.

Primary targets:

- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/alu.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`

Actions:

- Identify ALU-specific mnemonic, operand-width, extension, and special-case
  spelling bodies in the printer.
- Move ALU-specific spelling helpers into the ALU owner when they do not belong
  to a shared machine-printer table or family-neutral printer primitive.
- Preserve signed 32-bit extension, unsigned zero-extension, byte-swap,
  floating-negation, popcount, division, remainder, and variable-shift
  spelling behavior.
- Keep final assembly spelling derived from structured machine instruction
  data, not testcase text.

Completion check:

- A fresh build or delegated compile proof passes.
- `machine_printer.cpp` routes ALU-specific spelling to ALU-owned helpers or
  keeps only justified family-neutral printer code.
- Focused backend proof shows printed ALU behavior is preserved.

### Step 4: Reconcile `alu.md` And Close Shard Ownership

Goal: remove the markdown shard only after its durable behavior and risks are
represented in compiled ALU owners or in active execution notes.

Primary targets:

- `src/backend/mir/aarch64/codegen/alu.md`
- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/mir/aarch64/codegen/alu.hpp`
- `todo.md`

Actions:

- Re-check each `alu.md` entry point, dependency, hidden assumption, known
  failure risk, and rebuild-guidance item against compiled owners.
- Keep any remaining execution-only blocker in `todo.md`; do not delete
  `alu.md` while durable shard content is still unreconciled.
- Delete `src/backend/mir/aarch64/codegen/alu.md` only when the ALU compiled
  owners hold the valid durable content.
- Avoid carrying long prose from `alu.md` into code comments unless it is needed
  to explain a non-obvious invariant.

Completion check:

- `src/backend/mir/aarch64/codegen/alu.md` is deleted.
- `alu.cpp` and `alu.hpp` own the valid ALU shard behavior.
- No broad owner retains ALU-family bulk implementation without a recorded
  family-neutral justification.

### Step 5: Prove Behavior Preservation And Ownership Boundaries

Goal: validate that redistribution preserved ALU behavior and did not broaden
the slice beyond the source idea.

Primary targets:

- Focused AArch64 backend tests selected by the supervisor.
- Build or compile proof for touched backend code.
- Regression guard scope selected by the supervisor before lifecycle closure.

Actions:

- Run the supervisor-selected focused backend proof covering ALU lowering and
  printing behavior.
- Include nearby coverage for unsigned power-of-two reductions, register-direct
  ALU operations, accumulator fallback operations, signed 32-bit extension,
  byte-swap, floating negation, popcount, variable shifts, remainder, and i128
  copy when such coverage exists or is added as part of behavior preservation.
- Escalate to regression guard before closure or if multiple broad backend
  owners were changed.
- Record proof commands and outcomes in `todo.md`.

Completion check:

- Focused backend proof passes.
- Broader regression guard passes when required by the supervisor.
- The final diff contains ALU ownership redistribution and behavior-preserving
  proof, without unrelated feature expansion or expectation downgrades.
