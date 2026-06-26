Status: Active
Source Idea Path: ideas/open/391_rv64_variadic_prologue_save_area_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Capture The Variadic Save-Area Boundary

# Current Packet

## Just Finished

Lifecycle transition: idea 390 was closed as the prepared-call `va_list`
publication/copy owner, and idea 391 is now active for the later RV64
variadic prologue save-area publication boundary.

## Suggested Next

Start Step 1 by capturing prepared/BIR/object and disassembly evidence for
incoming RV64 variadic payload publication into the save area consumed by
`va_start` / `va_arg`.

## Watchouts

- Keep ideas 386, 387, 388, 389, and 390 out of scope unless new evidence
  proves this boundary belongs to one of those routes.
- Do not hard-code `va-arg-13.c`, `test`, `dummy`, registers, stack offsets,
  or the abort branch.
- Address or pointer materialization alone is not proof that the backing
  variadic payload save area was published.

## Proof

Lifecycle-only transition.

Close gate for idea 390:

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
- Result: PASS, 326/326 before and 326/326 after, no new failures.
