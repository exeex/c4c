# AArch64 Module Stage 3 Draft Review

## Decision

Accept the Stage 3 replacement draft set for implementation conversion.

The draft set is coherent enough to hand to the implementation-conversion
stage. No blocker or mandatory repair was found. The next stage should keep
the watchouts below visible while translating the drafts into real source and
build wiring.

## Reviewed Inputs

- `src/backend/mir/aarch64/module/stage2_review_layout.md`
- `src/backend/mir/aarch64/module/stage2_to_stage3_handoff.md`
- `src/backend/mir/aarch64/module/module.md`
- `src/backend/mir/aarch64/module/module.hpp.md`
- `src/backend/mir/aarch64/module/module.cpp.md`
- `src/backend/mir/aarch64/module/function_traversal.cpp.md`
- `src/backend/mir/aarch64/module/operand_resolution.cpp.md`
- `src/backend/mir/aarch64/module/instruction_lowering.cpp.md`
- `src/backend/mir/aarch64/module/branch_control_lowering.cpp.md`
- `src/backend/mir/aarch64/module/call_lowering.cpp.md`
- `src/backend/mir/aarch64/module/public_assembly_bridge.cpp.md`
- `src/backend/mir/aarch64/module/compatibility_projection.cpp.md`

## Stage 2 Handoff Compliance

The mandatory Stage 3 artifact map is complete:

- the directory index draft is present at `module.md`
- the single mandatory header draft is present at `module.hpp.md`
- all eight mandatory implementation drafts are present
- no extra non-helper `.hpp.md` draft exists

The drafts follow the Stage 2 sequencing and dependency direction:

`PreparedBirModule -> module dispatch -> function traversal -> operand
resolution -> instruction, branch/control, and call lowering -> canonical MIR
carrier -> public assembly bridge -> compatibility projection`.

The directory index and header establish the shared `MachineModule`,
`MachineFunction`, `MachineBlock`, `MachineInstruction`, `MachineOperand`,
`MachineRegister`, provenance, printer, and compatibility vocabulary used by
the implementation drafts. The implementation drafts then preserve their
assigned seams instead of reassembling the legacy catch-all emitter under
renamed helper boundaries.

## Parent 224 Constraint Check

The draft set preserves the parent 224 repair constraints:

- canonical output is hierarchical MIR organized by module, function, block,
  and instruction
- `FunctionRecord::machine_nodes` is retained only as a derived migration view
  in compatibility projection
- cached display strings, source spellings, and broad public records are
  rejected as instruction, operand, register, label, branch, call, data, or
  lowering authority
- the shared `mir_printer` owns traversal, ordering, section/file structure,
  labels, spacing, and emission mechanics
- AArch64 owns only target rendering hooks for instructions, operands,
  registers, memory, labels, symbols, immediates, relocation-aware forms, and
  target data fragments

No draft makes public records, flat vectors, cached display strings, spelling
recovery, or compatibility projection a semantic lowering input.

## Boundary Checks

- No target render API named `__repr__` is introduced; every mention is a
  rejection rule or explicit forbidden-knowledge statement.
- Raw prepared/source object retention is limited to diagnostic or
  compatibility provenance with authority limits.
- Register-string fallback is fail-closed diagnostic behavior, not structured
  register authority.
- Public assembly is downstream of completed canonical MIR and does not
  re-walk prepared lowering tables or compatibility records.
- Compatibility projection derives legacy records after canonical lowering and
  records removal conditions for retained migration views.
- Stage 3 remains markdown-only. This review did not require edits to real
  `.cpp` or `.hpp` files, CMake/build files, tests, or existing draft
  contents.

## Watchouts For Implementation Conversion

- Keep `module.hpp` as the single non-helper public header unless a later
  packet explicitly justifies private `helper.hpp` scope.
- Translate the drafts into real source in dependency order; do not start by
  preserving the legacy broad record assembler.
- Do not let compatibility projection become a fallback path for missing
  prepared authority during lowering.
- Keep return-value materialization, branch return control, and call result
  handling on their assigned seams during implementation.

## Repair Needs

None.
