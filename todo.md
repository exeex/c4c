Status: Active
Source Idea Path: ideas/open/216_aarch64_machine_node_asm_printer_external_smoke.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Add The Minimal AArch64 Machine-Node Printer

# Current Packet

## Just Finished

Step 2 added a codegen-owned AArch64 machine-node assembly printer over
selected structured `InstructionRecord` values.

Completed work:
- Added `print_machine_instruction_node(...)` and
  `print_machine_instruction_nodes(...)` under
  `src/backend/mir/aarch64/codegen/`.
- The printer requires `RecordSurfaceKind::MachineInstructionNode` and
  `MachineNodeSelectionStatus::Selected` before emitting any text.
- The first printable subset covers prepared spill/reload pseudo nodes as
  `str`/`ldr` against prepared frame-slot stack offsets, materialized-bool
  conditional branches as `cbnz` plus fallthrough `b`, unconditional branches
  as `b`, and selected stores whose value register and prepared address facts
  are structured.
- The printer fails closed for unselected nodes, external assembler/object
  surfaces, call/return placeholders, scalar nodes without a structured
  destination register, loads without a structured destination register, and
  unsupported or incomplete address forms.
- Added `backend_aarch64_machine_printer` focused coverage for accepted node
  printing and fail-closed cases without parsing printed assembly back into
  backend semantics.

## Suggested Next

Implement Step 3 by wiring the public `c4cll` assembly-output route for
AArch64 through source -> BIR -> prepared BIR -> AArch64 module ->
`FunctionRecord::machine_nodes` -> the new codegen printer, keeping route
diagnostics explicit when the current pipeline only supplies spill/reload
machine nodes.

## Watchouts

- The `.s` printer is output-only; do not parse printed assembly back into
  backend semantics.
- Do not bypass BIR/prepared BIR, AArch64 target MIR, or machine-node selection
  when serving the public CLI route.
- Do not implement an internal assembler, encoder, object writer, linker, or
  `.s` parser in this plan.
- Keep smoke proof rooted in the public `c4cll` route and the external
  toolchain when available.
- Distinguish record-level constructor coverage from the source-to-module
  pipeline: `aarch64::module::build(...)` currently appends only spill/reload
  machine nodes.
- Scalar records currently do not carry a destination register operand, and
  load records currently do not carry a destination register operand; the
  printer intentionally rejects those instead of guessing from value ids.

## Proof

Delegated proof passed and was written to `test_after.log`:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_aarch64_.*(machine|printer|target|memory|scalar|branch|prepared_frame|contract)') > test_after.log 2>&1`

Result: 17/17 selected backend AArch64 tests passed.
