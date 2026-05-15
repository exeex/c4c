# AArch64 Memory Load Store Machine Nodes Runbook

Status: Active
Source Idea: ideas/open/234_aarch64_memory_load_store_machine_nodes.md

## Purpose

Make AArch64 BIR load/store lowering reach selected `MemoryInstructionRecord`
machine nodes using prepared memory-access facts.

Goal: lower prepared local/global load and store instructions into structured
AArch64 memory machine records without reviving archived scratch-register
conventions or inferring address facts in target codegen.

Core Rule: AArch64 memory lowering must consume prepared address and register
authority. Do not synthesize frame-slot, pointer-value, symbol, or scratch
policy from instruction text or named test cases.

## Read First

- `ideas/open/234_aarch64_memory_load_store_machine_nodes.md`
- Current prepared memory-access facts and `PreparedMemoryAccess` lookup
  helpers.
- Current AArch64 memory operand records, `MemoryInstructionRecord`, dispatch
  memory diagnostics, and printer support.
- Existing global address materialization records, to keep memory access and
  address-producing materialization separate.

## Current Targets

- Selected AArch64 memory nodes for prepared BIR local/global loads.
- Selected AArch64 memory nodes for prepared BIR local/global stores.
- Structured load destination register facts.
- Prepared frame-slot and pointer-value base-plus-offset memory operands.
- Explicit diagnostics for unsupported symbol/string/unprepared bases.
- Focused backend tests proving semantic memory lowering, not named-case
  shortcuts.

## Non-Goals

- Do not reconstruct archived `memory.cpp` accumulator or scratch-register
  conventions.
- Do not infer prepared address facts inside AArch64 target codegen.
- Do not conflate global symbol memory access with global address
  materialization.
- Do not broaden into dynamic-stack/frame setup, memcpy, memset, F128
  full-width transport, or unrelated call lowering.
- Do not weaken backend expectations or mark supported load/store cases
  unsupported to claim progress.

## Working Model

- Prepared memory-access facts identify the authoritative address base,
  byte offset, size, alignment, volatility, and side-effect shape.
- Prepared value homes and regalloc facts identify load destination and store
  source registers when printable lowering is possible.
- AArch64 dispatch selects memory records only when both memory-address and
  register facts are complete.
- The terminal printer consumes selected memory records; it should not recover
  missing authority from BIR names or assembly text.
- Global address materialization remains a separate address-producing feature;
  global memory access should not reuse those records as a shortcut.

## Execution Rules

- Start by inspecting existing prepared memory carriers, AArch64 memory record
  fields, and dispatch/printer diagnostics.
- Keep each implementation packet narrow enough to prove with build plus
  focused backend tests.
- Add printer output only after the selected record carries every operand fact
  the printer needs.
- Preserve fail-closed diagnostics for unprepared or unsupported memory bases.
- Treat expectation rewrites, named-case matching, and fabricated register or
  frame-slot authority as route drift.

## Ordered Steps

### Step 1: Inspect Prepared Memory And AArch64 Surfaces

Goal: identify the exact carrier, dispatch, and printer gaps for load/store
memory nodes.

Primary Target: prepared memory-access facts, AArch64 memory operand builders,
`MemoryInstructionRecord`, block dispatch, and terminal printer paths.

Actions:

- Inspect how `PreparedMemoryAccess` records are produced and looked up for
  local/global load/store instructions.
- Inspect existing AArch64 memory operand records and which store path already
  prints.
- Identify what structured load destination register facts are missing.
- Identify current diagnostics for unsupported symbol/string or unprepared
  memory bases.
- Record the first implementation packet target in `todo.md` before code work.

Completion Check: the executor can name the exact first code packet and proof
subset without changing source intent.

### Step 2: Select Frame-Slot Load Nodes

Goal: lower prepared frame-slot loads into selected AArch64 memory records with
structured destination registers.

Primary Target: AArch64 memory dispatch and selected memory record
construction.

Actions:

- Consume prepared frame-slot read facts for BIR load instructions.
- Resolve the load destination from prepared value-home/regalloc authority.
- Preserve address size, alignment, volatility, byte offset, and frame-slot id
  in the selected record.
- Emit explicit missing-fact diagnostics when destination register or address
  authority is incomplete.

Completion Check: focused tests show a frame-slot load selected with a
structured destination register and prepared memory-read side effect.

### Step 3: Select Frame-Slot And Pointer-Value Store Nodes

Goal: lower prepared stores into selected AArch64 memory records when source
and address facts are complete.

Primary Target: AArch64 memory dispatch and selected memory record
construction.

Actions:

- Consume prepared frame-slot and pointer-value base-plus-offset write facts.
- Resolve the stored value from prepared value-home/regalloc authority.
- Preserve address size, alignment, volatility, byte offset, and base identity
  in the selected record.
- Keep unsupported symbol/string/unprepared bases deferred with explicit
  diagnostics.

Completion Check: focused tests show frame-slot and pointer-value stores
selected from prepared address and stored-value facts.

### Step 4: Print Structured Load/Store Subset

Goal: print valid AArch64 load/store assembly only from complete selected
memory records.

Primary Target: AArch64 terminal printer paths for selected memory records.

Actions:

- Print selected frame-slot loads using structured destination register and
  prepared memory operand fields.
- Print selected frame-slot and pointer-value stores using structured source
  register and memory operand fields.
- Preserve fail-closed diagnostics when required register or address fields are
  missing.
- Do not print symbol/string or unsupported bases through fallback templates.

Completion Check: printer output is driven by selected record fields and
missing facts produce explicit diagnostics.

### Step 5: Validate Semantic Coverage

Goal: prove nearby load/store cases without overfitting one fixture.

Primary Target: focused AArch64 backend tests plus the supervisor-selected
broader check.

Actions:

- Add or update tests for frame-slot load, frame-slot store, pointer-value
  store, and unsupported/unprepared base diagnostics.
- Verify global address materialization remains separate from global memory
  access.
- Run build proof and delegated focused backend tests after each code slice.
- Escalate to broader validation when the route activates multiple memory
  families or reaches a close-quality milestone.

Completion Check: supported prepared load/store cases pass through structured
lowering/printing or fail with explicit unsupported diagnostics; no supported
case depends on name-shaped matching.
