Status: Active
Source Idea Path: ideas/open/570_rv64_unsupported_instruction_fragment_owner_diagnostics.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Diagnostic Emission Paths

# Current Packet

## Just Finished

Activation created the runbook for
`ideas/open/570_rv64_unsupported_instruction_fragment_owner_diagnostics.md`.

## Suggested Next

Execute Step 1 by inventorying the current
`unsupported_instruction_fragment` emission paths, starting with
`c4c-clang-tools` queries around the RV64 object-emission diagnostic sites.

## Watchouts

- This plan is diagnostic-only; do not implement unsupported RV64 lowering
  operations discovered by the improved diagnostics.
- Keep the `unsupported_instruction_fragment` category stable while enriching
  the diagnostic context.
- Do not add named-case matching for the nine representative source files.
- Do not change expectations, unsupported markers, allowlists, or runtime
  comparison behavior.
- Use `.codex/skills/c4c-clang-tools` before broad C++ exploration in
  `object_emission.cpp`.

## Proof

Activation-only lifecycle work. Run
`git diff --check -- plan.md todo.md ideas/open/570_rv64_unsupported_instruction_fragment_owner_diagnostics.md`
before committing.
