Status: Active
Source Idea Path: ideas/open/227_aarch64_module_phoenix_replacement_drafts.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Draft Replacement Index And Header Contract

# Current Packet

## Just Finished

Completed plan Step 1 by rewriting
`src/backend/mir/aarch64/module/module.md` as the Stage 3 replacement
directory index and `src/backend/mir/aarch64/module/module.hpp.md` as the
single public header contract draft. The drafts establish the direct
prepared-BIR-to-typed-MIR boundary, canonical carrier vocabulary, lightweight
provenance model, compatibility-projection boundary, target-owned printable
surfaces, mandatory Stage 3 artifact map, no-extra-public-header rule, and
`helper.hpp` exception conditions.

## Suggested Next

Delegate Step 2 to draft `module.cpp.md` and `function_traversal.cpp.md`
against the Step 1 vocabulary.

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
- Step 2 drafts should use the Step 1 `MachineModule`, `MachineFunction`,
  `MachineBlock`, `MachineInstruction`, `MachineOperand`, `Provenance`, and
  `CompatibilityProjection` vocabulary instead of inventing a second carrier.

## Proof

Markdown-only proof written to `test_after.log` with concise `rg` checks over
`module.md` and `module.hpp.md`. No build was required by the delegated proof.
