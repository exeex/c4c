# AArch64 Memory Shard Redistribution Runbook

Status: Active
Source Idea: ideas/open/253_aarch64_memory_markdown_shard_implementation_redistribution.md

## Purpose

Reconcile the legacy `memory.md` shard into ordinary compiled AArch64 memory
owners without expanding memory semantics or leaving load/store, address, or
memory-operand behavior stranded in broad codegen files.

## Goal

Make `src/backend/mir/aarch64/codegen/memory.cpp` and
`src/backend/mir/aarch64/codegen/memory.hpp` the real home for valid
memory-family construction, lowering, helper, and spelling behavior described
by `src/backend/mir/aarch64/codegen/memory.md`.

## Core Rule

This route is ownership redistribution, not feature expansion. Preserve
existing memory behavior while moving memory-family bodies out of broad owners
and delete `memory.md` only after its durable content is reconciled into
compiled owners.

## Read First

- `ideas/open/253_aarch64_memory_markdown_shard_implementation_redistribution.md`
- `src/backend/mir/aarch64/codegen/memory.md`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.hpp`
- `src/backend/mir/ALLOCATION_CONTRACT.md`
- `src/backend/mir/AAPCS64_CALL_RETURN_FRAME_CONTRACT.md`

## Current Scope

- Create `memory.cpp` and `memory.hpp` as the compiled owner for valid memory
  shard behavior.
- Move load/store, address, memory operand, and memory-family selected-node
  construction, lowering, and spelling behavior from broad owners into the
  memory owner where the behavior belongs.
- Keep `instruction.cpp` focused on family-neutral instruction core.
- Keep `dispatch.cpp` as routing into the memory owner rather than a place
  that owns memory-family bodies.
- Keep `machine_printer.cpp` from retaining memory-specific spelling bodies
  when those can be owned through memory shard helpers.
- Preserve existing prepared memory facts, allocation-result facts, address
  spaces, volatility, spill/reload carriers, and selected memory instruction
  behavior without inventing missing carriers.

## Non-Goals

- Do not redistribute any other AArch64 codegen markdown shard.
- Do not redesign `instruction.hpp` or the broad instruction hierarchy solely
  for this shard.
- Do not expand load/store, address, stack, F128, memcpy, spill/reload, or
  dynamic-stack semantics beyond behavior-preserving relocation.
- Do not create spill/reload operations, outgoing call-area layout, frame
  layout, or scratch ownership locally in the memory owner.
- Do not downgrade tests, weaken expectations, or mark supported memory paths
  unsupported to make movement appear complete.
- Do not recreate a centralized record pile under a new abstraction name.

## Working Model

- `memory.md` is the durable legacy surface inventory and risk list for this
  redistribution.
- Current memory ownership is defined by prepared memory accesses, target-MIR
  memory operands, selected memory machine nodes, allocation-result records,
  reserved MIR scratch policy, `module::SpillReloadRecord`,
  `ALLOCATION_CONTRACT.md`, and `AAPCS64_CALL_RETURN_FRAME_CONTRACT.md`.
- Memory ownership includes ordinary load/store records, memory operands,
  address computation helpers, memory side effects, load/store spelling, and
  routing when those behaviors already exist in structured compiled code.
- Broad files may keep family-neutral data structures, dispatch switches, or
  printer primitives, but should route memory behavior to memory-owned helpers.
- Missing prepared facts for legacy F128, over-aligned alloca, dynamic stack,
  bytewise memcpy, or scratch-register ownership are carrier gaps, not
  permission for local text-emitter recovery.

## Execution Rules

- Keep each implementation step semantic and ownership-oriented; reject
  testcase-shaped matching or named-case shortcuts.
- Move one coherent memory-family surface at a time and keep build proof fresh
  after code movement.
- Prefer existing local helper patterns and namespace conventions in nearby
  shard owners such as `alu.*`, `comparison.*`, `calls.*`, `returns.*`, and
  `operands.*` over introducing a new ownership protocol.
- Do not let `dispatch.cpp`, `instruction.cpp`, or `machine_printer.cpp` grow
  new memory bodies while claiming redistribution progress.
- Record blockers in `todo.md` when a broad owner cannot be reduced without a
  larger shared-interface or carrier decision; do not rewrite the source idea
  for routine execution findings.
- For code-changing steps, run at least a fresh build or compile proof plus the
  supervisor-selected focused backend subset. Escalate to broader regression
  guard before closure or when touched surfaces cross backend contracts.

## Ordered Steps

### Step 1: Inspect Memory Ownership Surfaces

Goal: identify the memory behavior still owned by broad files and map it to
concrete `memory.cpp`/`memory.hpp` extraction targets.

Primary targets:

- `src/backend/mir/aarch64/codegen/memory.md`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.hpp`

Actions:

- Inventory memory-specific selected-node construction, operand preparation,
  load/store records, address helpers, side-effect records, and spelling
  helpers outside a memory owner.
- Separate family-neutral instruction, dispatch, and printing surfaces from
  memory-family body ownership.
- Compare existing compiled memory ownership against every durable behavior,
  hidden assumption, known failure risk, and rebuild-guidance item documented
  in `memory.md`.
- Identify the first narrow extraction packet and supervisor-selected proof
  command without editing implementation files in this step.

Completion check:

- `todo.md` records concrete memory extraction targets, any family-neutral
  boundaries that must remain outside the memory owner, and the narrow proof
  command selected by the supervisor.

### Step 2: Create Memory Owner And Move Operand Construction Routes

Goal: establish `memory.cpp` and `memory.hpp` as compiled memory-family owners
and relocate memory-specific operand and selected-node construction bodies
from broad owners.

Primary targets:

- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/memory.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`

Actions:

- Add `memory.cpp` and `memory.hpp` following nearby shard-owner conventions.
- Move memory-specific `MemoryOperand` and `MemoryInstructionRecord`
  construction helpers to memory-owned functions.
- Preserve prepared address, volatility, address-space, loaded/stored value,
  pointer-home, and allocation-result facts.
- Leave `instruction.cpp` with family-neutral instruction core and shared data
  only.
- Leave `dispatch.cpp` as a route into memory-owned helpers, not as the owner
  of memory body logic.

Completion check:

- A fresh build or delegated compile proof passes.
- `instruction.cpp` and `dispatch.cpp` no longer own the moved memory body
  logic.
- Existing memory behavior remains covered by the focused backend proof
  selected by the supervisor.

### Step 3: Move Memory Lowering And Address Helpers

Goal: relocate memory-family lowering helpers while keeping address and
scratch authority in prepared shared carriers.

Primary targets:

- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/memory.hpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/ALLOCATION_CONTRACT.md`
- `src/backend/mir/AAPCS64_CALL_RETURN_FRAME_CONTRACT.md`

Actions:

- Move existing memory-specific lowering helpers into the memory owner.
- Preserve direct stack-slot, indirect pointer-slot, and over-aligned alloca
  addressing as distinct paths where current structured facts support them.
- Preserve typed load/store width, sign-extension, zero-extension, volatility,
  and address-space decisions at the instruction-selection boundary.
- Preserve spill/reload, frame-relative, dynamic-stack, and call-boundary
  behavior only from existing structured facts.
- Treat absent F128, over-aligned alloca, dynamic-stack, bytewise memcpy, or
  scratch facts as carrier gaps to record in `todo.md`, not as local
  text-emitter recovery work.

Completion check:

- A fresh build or delegated compile proof passes.
- Memory lowering helpers live in the memory owner where valid.
- No local frame layout, scratch ownership, spill/reload creation, or
  assembly-text recovery was introduced.

### Step 4: Move Memory Spelling And Printer-Side Bodies

Goal: route memory-specific machine spelling through memory-owned helpers where
that ownership is valid.

Primary targets:

- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/memory.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.hpp`

Actions:

- Identify load, store, address, memory operand, side-effect, and
  spill/reload spelling bodies in the printer.
- Move memory-specific spelling helpers into the memory owner when they do not
  belong to a shared machine-printer primitive or table.
- Preserve final spelling from structured memory instruction data.
- Keep parsed mnemonic text and final assembly text out of semantic authority.

Completion check:

- A fresh build or delegated compile proof passes.
- `machine_printer.cpp` routes memory-specific spelling to memory-owned
  helpers or keeps only justified family-neutral printer code.
- Focused backend proof shows printed memory behavior is preserved.

### Step 5: Reconcile `memory.md` And Close Shard Ownership

Goal: remove the markdown shard only after its durable behavior and risks are
represented in compiled memory owners or in active execution notes.

Primary targets:

- `src/backend/mir/aarch64/codegen/memory.md`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/memory.hpp`
- `todo.md`

Actions:

- Re-check each `memory.md` entry point, load/store dispatch note,
  constant-offset note, addressing helper note, stack/GEP note, over-aligned
  alloca note, memcpy note, width/sign note, dependency, hidden assumption,
  known failure risk, and rebuild-guidance item against compiled owners.
- Keep any remaining execution-only blocker in `todo.md`; do not delete
  `memory.md` while durable shard content is still unreconciled.
- Delete `src/backend/mir/aarch64/codegen/memory.md` only when the memory
  compiled owners hold the valid durable content.
- Avoid carrying long prose from `memory.md` into code comments unless it is
  needed to explain a non-obvious invariant.

Completion check:

- `src/backend/mir/aarch64/codegen/memory.md` is deleted.
- `memory.cpp` and `memory.hpp` own the valid memory shard behavior.
- No broad owner retains memory-family bulk implementation without a recorded
  family-neutral justification.

### Step 6: Prove Behavior Preservation And Ownership Boundaries

Goal: validate that redistribution preserved memory behavior and did not
broaden the slice beyond the source idea.

Primary targets:

- Focused AArch64 backend tests selected by the supervisor.
- Build or compile proof for touched backend code.
- Regression guard scope selected by the supervisor before lifecycle closure.

Actions:

- Run the supervisor-selected focused backend proof covering memory lowering
  and printing behavior.
- Include nearby coverage for ordinary load/store, direct stack-slot,
  indirect pointer-slot, over-aligned alloca, GEP, dynamic stack, F128, memcpy,
  spill/reload, volatility, and address-space behavior when such coverage
  exists or is added as part of behavior preservation.
- Escalate to regression guard before closure or if multiple broad backend
  owners were changed.
- Record proof commands and outcomes in `todo.md`.

Completion check:

- Focused backend proof passes.
- Broader regression guard passes when required by the supervisor.
- The final diff contains memory ownership redistribution and
  behavior-preserving proof, without unrelated feature expansion or
  expectation downgrades.
