Status: Active
Source Idea Path: ideas/open/229_aarch64_codegen_markdown_shards_to_cpp.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Establish Current Owner Map And Ledger Location

# Current Packet

## Just Finished

Completed `plan.md` Step 1 inventory setup for the current post-224/post-228
AArch64 codegen surfaces.

Live owner map:

- ALU: `src/backend/mir/aarch64/codegen/alu.cpp` / `.hpp` own scalar lowering
  state, scalar register-view selection, emitted-scalar register tracking, and
  `lower_scalar_instruction`.
- Comparison: `src/backend/mir/aarch64/codegen/comparison.cpp` / `.hpp` own
  prepared unconditional and conditional branch lowering plus block successor
  construction.
- Returns: `src/backend/mir/aarch64/codegen/returns.cpp` / `.hpp` own prepared
  return terminator lowering, including scalar-state lookup for named return
  values.
- Emit: `src/backend/mir/aarch64/codegen/emit.cpp` / `.hpp` own the AArch64
  codegen `build_module` entry point.
- Dispatch: `src/backend/mir/aarch64/codegen/dispatch.cpp` / `.hpp` own block
  lowering context construction, instruction-family classification, per-block
  prepared instruction dispatch, and terminator dispatch.
- Instruction records: `src/backend/mir/aarch64/codegen/instruction.cpp` /
  `.hpp` own AArch64 target instruction record shapes, record surface kinds,
  machine opcodes, selection status, side effects/effects, prepared branch,
  scalar ALU, scalar cast, memory, call, return, assembler, object, and
  unsupported record construction.
- Operands: `src/backend/mir/aarch64/codegen/operands.cpp` / `.hpp` own
  resolved MIR operands and operand authority from regalloc, storage plan,
  prepared value homes, immediates, labels, and symbols.
- Traversal: `src/backend/mir/aarch64/codegen/traversal.cpp` / `.hpp` own
  prepared function traversal, function lowering context construction, MIR
  function/block population, and dispatch fan-out.
- Machine printing: `src/backend/mir/aarch64/codegen/machine_printer.cpp` /
  `.hpp` own AArch64 target instruction spelling via `MachineInstructionPrinter`
  and per-family line payload printers for selected machine nodes.
- Module projection: `src/backend/mir/aarch64/module/module.cpp` / `.hpp` own
  the public AArch64 module build facade, `module::Module`, diagnostics, and
  aliases from shared MIR containers to AArch64 `InstructionRecord` payloads.
- Compatibility projection: `src/backend/mir/aarch64/codegen/compatibility_projection.cpp`
  / `.hpp` own legacy flat function/machine-node views for tests and migration
  callers only.
- Shared MIR container and printer boundary: `src/backend/mir/mir.hpp` owns
  `MachineModule`, `MachineFunction`, `MachineBlock`, `MachineInstruction`,
  common operands, origins, successors, walkers, and compatibility flattening;
  `src/backend/mir/printer.hpp` / `.cpp` own shared module/function walking and
  generic print failure policy. AArch64 supplies only the target printer.

Ledger location: keep the active shard reconciliation ledger directly in this
`todo.md` current packet while the runbook is active. Do not create a separate
artifact for the ledger unless it outgrows this execution scratchpad or the
supervisor delegates one; `review/` remains transient reviewer output, not a
ledger home.

## Suggested Next

Execute `plan.md` Step 2 by reconciling only the active compiled surface shard
group: `alu.md`, `comparison.md`, `returns.md`, and `emit.md`.

First proof/evidence strategy for Step 2:

- For each shard, compare the markdown intent against the live owner file pair
  named above before assigning a classification.
- Use code references from `rg`/`sed` inventory as primary evidence:
  `lower_scalar_instruction`, prepared branch terminator lowering, prepared
  return terminator lowering, and `build_module`.
- Use the existing focused backend tests as supporting evidence when a shard
  maps to compiled behavior: scalar ALU records, branch/compare records,
  return lowering, instruction dispatch, function traversal, machine printer,
  and module skeleton tests under `tests/backend/mir/`.
- Do not run broad validation for ledger-only Step 2 classification. If Step 2
  discovers a code-changing gap, stop for a bounded implementation packet with
  a supervisor-selected proof command.

## Watchouts

- Do not mechanically convert markdown shards into same-named C++ files.
- Do not reopen the shared MIR printer boundary closed by idea 224.
- Do not treat `records.md` as a mandate to recreate legacy record ownership.
- Do not classify a missing feature before checking current compiled owners.
- Keep compatibility projection out of terminal assembly printing; terminal
  assembly must walk shared `module::MachineModule` through
  `mir::print_machine_module` plus the AArch64 `MachineInstructionPrinter`.
- Step 1 intentionally did not classify individual shards.

## Proof

Lifecycle/documentation-only inventory; no build or test proof required, and no
broad validation was run. Evidence inspected with file inventory and focused
source reads:

- `find src/backend/mir/aarch64/codegen -maxdepth 1 -type f | sort`
- `rg --files src tests | rg 'AArch64|aarch64|MIR|Machine|Instruction|Operand|Print|Emit|Dispatch|Return|Compare|ALU|Module|Compat|Traversal|isel|lower'`
- `sed` reads of the live AArch64 codegen/module headers and shared
  `src/backend/mir/mir.hpp` / `src/backend/mir/printer.hpp`
- `rg` function inventory across the live AArch64 codegen `.cpp` files
