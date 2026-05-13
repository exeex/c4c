Status: Active
Source Idea Path: ideas/open/215_aarch64_first_machine_node_selection_slice.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Add Minimal Machine Node Model

# Current Packet

## Just Finished

Completed `plan.md` Step 2: Add Minimal Machine Node Model.

The existing `InstructionRecord` shell in
`src/backend/mir/aarch64/codegen/records.hpp` now carries the minimal selected
machine-node surface for the first AArch64 selection subset:

- concrete `MachineOpcode` identity for branch, compare-branch, scalar ALU,
  simple cast, memory load/store, and spill/reload pseudo op families
- explicit `MachinePseudoKind` markers for future selected spill/reload pseudo
  nodes
- `MachineNodeStatusRecord` with selected/deferred/missing-facts status and a
  diagnostic string for fail-closed unsupported cases
- ordered typed node operands copied from existing branch/scalar/memory record
  payloads without assembly text
- `defs`, `uses`, `clobbers`, and `side_effects` vectors over structured
  resources such as prepared values, registers, memory, frame slots, symbols,
  branch targets, and control-flow/memory/call/return effects

`records.cpp` now derives opcode, ordered operands, def/use metadata, and
side-effect metadata in the existing `make_*_instruction` helpers. It also
publishes name helpers for the new enums and a
`make_unsupported_machine_instruction(...)` fail-closed constructor for
unsupported selection inputs.

Focused backend tests now inspect the new structured fields directly in
`backend_aarch64_target_instruction_records`,
`backend_aarch64_scalar_record_contract`, and
`backend_aarch64_memory_operand_records`, including selected status, concrete
opcode names, pseudo vocabulary, def/use resources, memory/control-flow
effects, and deferred unsupported status without relying on assembly text.

## Suggested Next

Implement Step 3 by selecting scalar ALU/cast and branch/compare records into
the now-structured `InstructionRecord` machine-node fields, keeping unsupported
forms fail-closed through `MachineNodeStatusRecord`.

## Watchouts

- Do not emit or parse assembly text under this plan.
- Select only from accepted target-MIR records; do not bypass through rendered
  BIR, old examples, or fixture names.
- Step 2 added the model and helper derivation, not a module-owned selected-node
  list.
- Keep call/return/prologue/variadic/global/linker behavior out of the first
  selection subset even though placeholder record variants and statuses exist.
- The spill/reload pseudo vocabulary is present, but the module spill/reload
  bridge still belongs to the later spill/reload selection packet.

## Proof

Ran delegated proof:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: passed. The enabled `^backend_` subset reported 100% passing for 131
enabled backend tests.

Log path: `test_after.log`.

Also ran `git diff --check`: passed. `clang-format` is not installed in this
environment.
