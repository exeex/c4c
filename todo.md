Status: Active
Source Idea Path: ideas/open/384_aarch64_dispatch_mechanical_extraction.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Extract Dynamic Stack and Entry Formal Helpers

# Current Packet

## Just Finished

Activation created the runbook and initialized execution state for Step 1.

## Suggested Next

Execute Step 1 from `plan.md`: use AST-backed queries on
`src/backend/mir/aarch64/codegen/dispatch.cpp`, then mechanically extract the
dynamic stack and entry formal helper clusters into build-wired `dispatch_*`
files while preserving behavior.

## Watchouts

- Use `.codex/skills/c4c-clang-tools` before moving code.
- Keep extraction mechanical; do not change dispatch order, generated assembly,
  MIR behavior, or test expectations.
- Record any grouping adjustment here before making larger ownership changes.

## Proof

Required minimum proof after the first extraction packet:

```bash
cmake --build build --target c4cll backend_aarch64_instruction_dispatch_test -j10
ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_lir_to_bir_notes|c_testsuite_aarch64_backend_src_00204_c)$' --output-on-failure
```
