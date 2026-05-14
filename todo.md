Status: Active
Source Idea Path: ideas/open/227_aarch64_module_phoenix_replacement_drafts.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Draft Dispatch And Traversal Seams

# Current Packet

## Just Finished

Completed plan Step 2 by rewriting
`src/backend/mir/aarch64/module/module.cpp.md` as the Stage 3 module dispatch
replacement draft and adding
`src/backend/mir/aarch64/module/function_traversal.cpp.md` as the Stage 3
function traversal replacement draft. The drafts use the Step 1
`MachineModule`, `MachineFunction`, `MachineBlock`, `MachineInstruction`,
`MachineOperand`, and lightweight `Provenance` vocabulary; cover target
profile resolution, AArch64 prepared-handoff validation, module data and
relocation orchestration, `BuildResult` construction, public product assembly
from completed canonical MIR functions, prepared function/block traversal,
lowering context construction, MIR function/block creation, optional debug
provenance, owned inputs/outputs, indirect queries, forbidden knowledge, and
classification; and reject the legacy broad record assembler as the
replacement driver.

## Suggested Next

Delegate Step 3 to draft `operand_resolution.cpp.md` against the Step 1
carrier/provenance vocabulary and the Step 2 dispatch/traversal seams.

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
- Step 3 should keep storage precedence inside operand resolution so
  instruction, branch, and call lowering do not choose among broad optional
  public records.
- Register spelling fallback belongs only in fail-closed compatibility
  diagnostics; structured prepared facts must remain authoritative.
- Do not let operand resolution reintroduce public `FunctionRecord` records or
  flat `machine_nodes` as semantic inputs.

## Proof

Markdown-only proof written to `test_after.log` with concise read-only `rg`
checks over `module.cpp.md` and `function_traversal.cpp.md`. The proof confirms
the drafts exist, use the Step 1 prepared-BIR-to-typed-MIR vocabulary, keep the
shared `mir_printer` boundary, avoid `FunctionRecord::machine_nodes` as a
primary output, avoid `__repr__`, and did not require real source, build, or
test edits.
