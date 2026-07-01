Status: Active
Source Idea Path: ideas/open/484_bir_semantic_local_address_provenance_array_element_authority.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Package Local-Address Provenance From Checker Inputs

# Current Packet

## Just Finished

Lifecycle switched from completed idea 486 back to reopened idea 484. Idea 486
closed after publishing `local_array_index_range_checker_inputs`; idea 484 can
now resume local-address/array-element provenance packaging from that checker
input surface.

## Suggested Next

Implement Step 5 by packaging matching `local_array_index_range_checker_inputs`
into the Step 2 local-address/array-element provenance authority record, then
prove available and fail-closed records through focused backend coverage.

## Watchouts

- Consume `local_array_index_range_checker_inputs`; do not re-derive from
  proof facts, range certificates, selected paths, interval effects, endpoint
  bridges, final homes, raw testcase shape, or synthetic effect inputs.
- Preserve fail-closed distinctions for missing/non-available checker inputs,
  missing source object, missing derivation, missing dynamic index, missing
  range, out-of-bounds, aggregate/member, integer-pointer round-trip, global,
  variadic/runtime, unsupported type, bootstrap, raw-shape-only, target-only,
  and coordinate confusion.
- Scalar local-load production, RV64/MIR lowering, object emission, and broad
  generic provenance analysis remain out of scope.

## Proof

Lifecycle switch proof:

```sh
git diff --check > test_after.log 2>&1 && python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
