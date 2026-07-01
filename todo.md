Status: Active
Source Idea Path: ideas/open/483_bir_semantic_local_memory_scalar_load_producer.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Produce Scalar Local Loads From Provenance

# Current Packet

## Just Finished

Lifecycle switched from completed idea 484 back to reopened idea 483. Idea 484
closed after publishing `local_array_local_address_provenances`; idea 483 can
now resume scalar local-load production from that provenance surface.

## Suggested Next

Implement Step 2 by producing semantic scalar local-memory load facts from
matching `local_array_local_address_provenances`, then prove available and
fail-closed facts through focused backend coverage.

## Watchouts

- Consume `local_array_local_address_provenances`; do not re-derive provenance
  from checker inputs, proof facts, range certificates, selected paths,
  interval effects, endpoint bridges, final homes, raw testcase shape,
  synthetic effect inputs, or target behavior.
- Preserve fail-closed distinctions for missing/non-available provenance,
  aggregate/member, integer-pointer round-trip, global source object,
  variadic/runtime boundary, unsupported type, bootstrap boundary,
  raw-shape-only evidence, target-only evidence, and coordinate confusion.
- RV64/MIR lowering, object emission, aggregate/member producers,
  variadic/byval/va_arg work, volatile/atomic work, complex/vector/F128 work,
  and broad generic load analysis remain out of scope.

## Proof

Lifecycle switch proof:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed > test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
