Status: Active
Source Idea Path: ideas/open/160_sema_canonical_symbol_template_key_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Acquire Accepted Closure Regression Gate

# Current Packet

## Just Finished

Step 1 is complete. Acquired canonical close-scope `test_after.log` for the
accepted closure subset after a fresh build. The selected sema metadata tests
passed: `cpp_hir_sema_canonical_symbol_structured_metadata`,
`cpp_hir_sema_consteval_type_utils_structured_metadata`, and
`cpp_positive_sema_lookup_value_structured_metadata`. The command-matched
regression guard also passed for the accepted close scope: before and after
both passed 3/3, with no new failures and no new tests over 30 seconds.

## Suggested Next

Proceed to Step 2 / close-or-reassess. If the source idea acceptance criteria
remain satisfied, ask the plan owner to close the idea; otherwise reassess the
remaining lifecycle state before closure.

## Watchouts

- Do not move the source idea to `ideas/closed/` until the close-time
  regression guard passes.
- Do not accept `test_baseline.new.log` as the full-suite baseline with the
  three known unrelated failures still present.
- The remaining active work is closure validation only, not implementation.
- Retained rendered-string fallbacks from the completed route remain explicit
  compatibility/output bridges: diagnostic, debug, ABI, and no-metadata display
  strings may remain, but covered semantic identity paths must use structured
  metadata.

## Proof

Last accepted supporting proof:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_sema_canonical_symbol_structured_metadata|cpp_hir_sema_consteval_type_utils_structured_metadata|cpp_positive_sema_lookup_value_structured_metadata)$'`

Result: passed as closure-scope after proof.

Regression guard:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: passed for the accepted close scope.

Proof log: `test_after.log`.
