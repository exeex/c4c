Status: Active
Source Idea Path: ideas/open/408_prepared_va_start_destination_address_helper_operand_publication.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Classify The Missing Helper Operand Publication

# Current Packet

## Just Finished

Lifecycle review closed 398 as stale/passing for current RV64 object-route
stack-frame, callee-saved, parameter-home, and function-admission diagnostics.
The remaining `va-arg-21.c` evidence was split into this producer-side
`va_start` helper operand publication idea.

## Suggested Next

Execute Step 1: inspect the fresh `va-arg-21.c` prepared dump and classify why
`helper_operand_homes.va_start.destination_va_list_address` is missing even
though the variadic entry plan, `va_list` layout, overflow-area state,
incoming variadic GPR publications, and `llvm.va_start.p0` call sites are
present.

## Watchouts

- Do not infer the destination `va_list` address in RV64 object emission.
- Keep destination `va_list` storage distinct from destination `va_list`
  address metadata.
- Preserve precise unsupported diagnostics for unsupported helper operand
  shapes.
- Do not use filename-specific handling, expectation rewrites, or allowlist
  filtering as progress.

## Proof

Lifecycle close/switch proof:

- Close gate: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
- Regression guard:
  `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
- Result: PASS, 326/326 before and 326/326 after, no new failures.
