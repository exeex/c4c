Status: Active
Source Idea Path: ideas/open/227_aarch64_module_phoenix_replacement_drafts.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Draft Instruction And Control Lowering

# Current Packet

## Just Finished

Completed plan Step 4 by adding
`src/backend/mir/aarch64/module/instruction_lowering.cpp.md` and
`src/backend/mir/aarch64/module/branch_control_lowering.cpp.md` as Stage 3
replacement drafts. The drafts lower prepared instruction and control-flow
semantics into canonical MIR `MachineInstruction` nodes and explicit successor
metadata using typed operands, prepared operation/terminator authority, owned
inputs/outputs, indirect queries, forbidden knowledge, classification, and
fail-closed rejection of named-testcase shortcuts, cached display strings,
source spellings, public branch records, compatibility projections, and flat
`FunctionRecord::machine_nodes` authority.

## Suggested Next

Delegate Step 5 to draft call lowering against the typed operand authority
seam, keeping ABI call sequencing out of ordinary instruction and
branch/control lowering.

## Watchouts

- Stage 3 is draft-only; do not edit real `.cpp`, `.hpp`, build, or test files.
- Do not delete, disable, or build-disconnect the compiled legacy `module.cpp`.
- Do not add component-level public headers. `helper.hpp` is the only allowed
  header exception and must be justified before drafting.
- Do not make `FunctionRecord::machine_nodes`, cached display strings, spelling
  recovery, or broad public inspection records semantic lowering authority.
- Do not introduce a target render API named `__repr__`.
- If the Stage 2 artifact map appears wrong, stop and report a Stage 2 contract
  repair blocker instead of silently adding or removing files.
- Step 5 should consume prepared call plans, ABI locations, clobbers,
  preserved values, indirect callees, and memory-return facts through the typed
  operand seam rather than rechecking broad public records.
- Keep call-adjacent moves in call lowering when they are governed by the call
  plan; ordinary move families already belong to instruction lowering.
- Compatibility call records should remain derived projections after canonical
  MIR call sequences exist.

## Proof

Markdown-only proof written to `test_after.log` with concise read-only `rg`
checks over `instruction_lowering.cpp.md` and
`branch_control_lowering.cpp.md`. The proof confirms both files are Stage 3
replacement drafts; instruction lowering covers scalar ALU, memory, moves,
spill/reload, parallel-copy, return-value materialization, canonical
`MachineInstruction` output, typed operand consumption, owned inputs/outputs,
indirect queries, forbidden knowledge, classification, and rejection of
named-testcase, cached-display, compatibility-projection, source-spelling, and
flat `machine_nodes` authority. Branch/control lowering covers prepared
terminators, conditional/unconditional branches, compare/condition handling,
valid branch fusion, return control flow, successor metadata, typed
operands/labels, owned inputs/outputs, indirect queries, forbidden knowledge,
classification, and rejection of public branch records/source spellings as
authority. No build was required for this markdown-only draft packet.
