Status: Active
Source Idea Path: ideas/open/227_aarch64_module_phoenix_replacement_drafts.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Draft Public Bridge And Compatibility Projection

# Current Packet

## Just Finished

Completed plan Step 6 by adding
`src/backend/mir/aarch64/module/public_assembly_bridge.cpp.md` and
`src/backend/mir/aarch64/module/compatibility_projection.cpp.md` as Stage 3
replacement drafts. The public bridge routes canonical `MachineModule`,
function, block, instruction, module-data, and relocation products to the
shared `mir_printer`, with AArch64 owning only target render hooks. The
compatibility projection derives `FunctionRecord::machine_nodes`, broad
inspection records, provenance views, label views, and legacy register-string
diagnostics after canonical lowering, with removal conditions and no semantic
lowering authority.

## Suggested Next

Delegate Step 7 to review the complete Stage 3 replacement draft set against
the Stage 2 handoff and parent 224 boundary.

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
- Step 7 should reject any draft that makes compatibility records, cached
  strings, source spellings, flat `machine_nodes`, or public bridge output a
  semantic lowering input.

## Proof

Markdown-only proof written to `test_after.log` with concise read-only `rg`
checks over both Step 6 drafts. The proof confirms both files are Stage 3
replacement drafts. Public assembly bridge coverage includes canonical
MIR/module data to shared `mir_printer`, AArch64 target-owned rendering hooks,
`.s` emission through common traversal, owned inputs/outputs, indirect
queries, forbidden knowledge, classification, and rejection of AArch64-owned
traversal, cached string printing, and compatibility-vector printing.
Compatibility projection coverage includes derived
`FunctionRecord::machine_nodes`, broad inspection records, raw
source/prepared provenance, label views, legacy register-string diagnostics,
removal conditions, owned inputs/outputs, indirect queries, forbidden
knowledge, classification, and rejection of projection as semantic lowering
authority. No build was required for this markdown-only draft packet.
