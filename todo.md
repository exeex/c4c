Status: Active
Source Idea Path: ideas/open/162_link_name_id_backend_symbol_authority.md
Source Plan Path: plan.md
Current Step ID: 8
Current Step Title: Document Retained Compatibility Bridges

# Current Packet

## Just Finished

Step 6/7 proof is complete through commit `2a7af71f3` (`Prove BIR initializer
LinkNameId validation`). The remaining focused `backend_lir_to_bir_notes`
verifier coverage proves BIR initializer-symbol LinkNameId boundaries:
raw-only initializer symbols remain compatibility-valid only without
LinkNameId, declared global/function LinkNameIds validate even when retained
display spelling drifts, unknown LinkNameIds reject before raw-name lookup, and
known-but-undeclared LinkNameIds fail closed instead of falling back to matching
raw spelling. Supervisor full-suite monotonic regression proof also passed with
no new failures.

## Suggested Next

Execute Step 8: document retained compatibility bridges and prepare closure
review. Confirm remaining raw string paths are output, diagnostics, route-local
names, or explicitly documented compatibility/no-metadata bridges; then record
whether the source idea acceptance criteria are satisfied or whether another
explicit implementation packet remains.

## Watchouts

- No production validator change was needed; existing global load/store
  validation and initializer-symbol validation already use `LinkNameId` as
  authoritative when present.
- Raw same-module global names and initializer-symbol spellings remain accepted
  only for compatibility contracts where the BIR reference has an invalid
  `LinkNameId`.
- Step 8 should not expand into new symbol-authority implementation unless it
  identifies a concrete uncovered route; capture documentation/closure notes in
  `todo.md` first.
- The untracked `review/` artifacts were not touched.

## Proof

Passed:

```sh
{ cmake --build --preset default --target backend_lir_to_bir_notes_test backend_prepare_frame_stack_call_contract_test backend_prepare_stack_layout_test backend_prepare_authoritative_join_ownership_test backend_prepare_structured_context_test frontend_hir_tests && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_prepare_frame_stack_call_contract|backend_prepare_stack_layout|backend_prepare_authoritative_join_ownership|backend_prepare_structured_context|frontend_hir_tests)$'; } > test_after.log 2>&1
```

Proof log: `test_after.log`. Result: build succeeded and 6/6 selected tests
passed.

Supervisor full-suite proof after commit `2a7af71f3`:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure > test_after.log 2>&1
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_baseline.log --after test_after.log --allow-non-decreasing-passed
```

Result: PASS. Baseline `test_baseline.log`: 3104 passed, 31 failed, 3135
total. After `test_after.log`: 3104 passed, 31 failed, 3135 total. No new
failures.
