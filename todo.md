Status: Active
Source Idea Path: ideas/open/240_aarch64_inline_asm_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Broader Backend Validation And Closure Readiness

# Current Packet

## Just Finished

Completed Plan Step 6 broader backend validation and closure-readiness review
for the AArch64 inline-asm machine-node route.

Implemented:

- Ran the supervisor-selected broader backend validation for the active route.
- Compared matching backend before/after proof logs with the regression guard.
- Reviewed remaining gaps against the source idea and current runbook.
- The source idea is not closure-ready yet because named operand substitution,
  structured clobber authority, memory/address constraints, and allocator-scale
  tied-output/coallocation policy remain unsupported or diagnostic-only.

## Suggested Next

Ask the plan owner to decide whether to replace the exhausted runbook with a
follow-up plan or split the remaining durable scope into one or more
`ideas/open/` files. Do not close the source idea as complete.

## Watchouts

- Current implemented support covers structured positional register input,
  one register output, numeric tie facts, integer immediates, `%wN`/`%xN`
  register-width modifiers, side-effect marking, and explicit diagnostics for
  unsupported forms.
- Named operand references are still fail-closed; retained operand names are
  carried but not substituted from `%[name]`.
- Clobbers remain diagnostic-only because source/LIR does not provide
  structured clobber vectors.
- Memory/address constraints and allocator-scale tie/coallocation policy remain
  out of scope for the completed runbook.

## Proof

Ran the delegated proof command:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed. `test_after.log` contains the build plus 139/139 backend tests
passing.

Regression guard:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: passed. Before and after both report 139/139 backend tests passing,
with no new failures and no new slow-test entries.
