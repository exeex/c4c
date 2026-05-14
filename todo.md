Status: Active
Source Idea Path: ideas/open/227_aarch64_module_phoenix_replacement_drafts.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Draft Call Lowering

# Current Packet

## Just Finished

Completed plan Step 5 by adding
`src/backend/mir/aarch64/module/call_lowering.cpp.md` as a Stage 3
replacement draft. The draft lowers prepared call plans into canonical MIR
`MachineInstruction` call sequences while keeping call-site ABI sequencing in
the call-lowering seam. It covers argument and result locations,
call-adjacent moves, preserved values, clobbers, indirect callees, memory
returns, call-site ABI bindings, typed operand consumption, owned
inputs/outputs, indirect queries, forbidden knowledge, classification, and
fail-closed rejection of broad public call records, compatibility projections,
cached display strings, source spellings, named-testcase call handling, and
flat `FunctionRecord::machine_nodes` authority.

## Suggested Next

Delegate Step 6 to draft the public assembly bridge and compatibility
projection after canonical MIR lowering exists.

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
- Step 6 bridge/projection drafts should derive public assembly and
  compatibility views from canonical `MachineModule`, `MachineFunction`,
  `MachineBlock`, `MachineInstruction`, typed operands, prepared ABI facts,
  and provenance after lowering.
- The public bridge must route traversal and `.s` file structure through the
  shared `mir_printer`; AArch64 should own only target rendering hooks.
- Compatibility projection may expose broad public records and flat
  `FunctionRecord::machine_nodes`, but only as migration views derived after
  canonical lowering.

## Proof

Markdown-only proof written to `test_after.log` with concise read-only `rg`
checks over `call_lowering.cpp.md`. The proof confirms the file is a Stage 3
replacement draft and covers prepared call-plan lowering, argument/result
locations, call-adjacent moves, preserved values, clobbers, indirect callees,
memory returns, call-site ABI bindings, typed operand consumption, canonical
MIR call sequence output, owned inputs/outputs, indirect queries, forbidden
knowledge, classification, and rejection of broad public call records,
compatibility projections, cached-display/source-spelling authority, and flat
`machine_nodes` authority. No build was required for this markdown-only draft
packet.
