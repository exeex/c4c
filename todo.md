Status: Active
Source Idea Path: ideas/open/291_retire_rendered_call_arg_abi_suffix_fallback.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Quarantine or remove legacy rendered fallback

# Current Packet

## Just Finished

Step 3 from `plan.md` completed: quarantined the remaining
`strip_call_arg_abi_type_suffix` call behind the explicit
`legacy_raw_no_ref_call_arg_type_text` helper.

Concrete changes:

- `src/backend/bir/lir_to_bir/calling.cpp`: added an auditable
  `legacy_raw_no_ref_call_arg_fallback_allowed` predicate. Rendered suffix
  stripping is now allowed only when the argument has no structured type-ref
  carrier and no structured ABI payload such as HFA lane metadata or
  `aarch64_stack_align_bytes`.
- `src/backend/bir/lir_to_bir/calling.cpp`: changed scalar call-argument
  pre-classification to fail closed when neither structured type metadata nor
  the raw/no-ref legacy predicate is available.
- `src/backend/bir/lir_to_bir/calling.cpp`: changed the byval layout legacy
  branches to route through the same predicate, so metadata-bearing or
  mismatched structured arguments cannot be rescued by rendered text.

## Suggested Next

Execute Step 4 from `plan.md`: add focused proof coverage for the structured
metadata boundary and any retained legacy raw/no-ref compatibility behavior,
then run the existing 286/288 proof subset named in the plan.

## Watchouts

The remaining rendered suffix parser inventory is now two lines in
`calling.cpp`: the `strip_call_arg_abi_type_suffix` definition and a single
call inside `legacy_raw_no_ref_call_arg_type_text`. That remaining call is
justified only for legacy/raw no-ref compatibility. Existing metadata-rich
byval mismatch tests still prove structured mismatch fails closed; Step 4
should add or tighten focused coverage for the new predicate if the supervisor
wants direct assertions beyond the current notes subset.

Supervisor broader guard context remains unchanged: the previous broader
`^backend_` before/after guard was non-regressive but had 21 pre-existing
failures. Do not fix those in the next packet unless explicitly delegated.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_notes$' && rg -n "strip_call_arg_abi_type_suffix" src/backend/bir/lir_to_bir/calling.cpp`

Result: build completed, `backend_lir_to_bir_notes` passed, and `rg` reported
only the helper definition plus the single legacy helper call. Full output is
preserved in `test_after.log`.

Supervisor broader guard:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: non-regressive before/after comparison with
`check_monotonic_regression.py --allow-non-decreasing-passed`; both runs had
159 passed, 21 failed, 180 total, with no new failures. The 21 failures are
the same pre-existing backend CLI/route semantic BIR admission failures noted
after Step 2.
