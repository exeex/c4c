Status: Active
Source Idea Path: ideas/open/484_bir_semantic_local_address_provenance_array_element_authority.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Package Local-Address Provenance From Checker Inputs

# Current Packet

## Just Finished

Implemented Step 5: production dynamic local-array local-address provenance now
publishes through `local_array_local_address_provenances` only from matching
available `local_array_index_range_checker_inputs`. The record packages source
object, derivation, element path, dynamic index, layout/range, checker status,
and producer coordinate identity, and fails closed for missing/non-available
checker inputs, missing source object, missing derivation, missing dynamic
index, missing range, out-of-bounds element paths, aggregate/member,
integer-pointer round-trip, global, variadic/runtime, unsupported type,
bootstrap, raw-shape-only, target-only, and coordinate-confusion
representatives.

## Suggested Next

Execute Step 6 from `plan.md`: record whether idea 483 scalar local-load
production can resume from `local_array_local_address_provenances`.

## Watchouts

- Consumers should use `local_array_local_address_provenances`; they should not
  re-derive from checker inputs, proof facts, range certificates, selected
  paths, interval effects, endpoint bridges, final homes, raw testcase shape, or
  synthetic effect inputs.
- Scalar local-load production, RV64/MIR lowering, object emission, and broad
  generic provenance analysis were not implemented in this packet.

## Proof

Delegated Step 5 proof:

```sh
cmake --build build -j2 > test_after.log 2>&1 && ctest --test-dir build -j2 --output-on-failure -R '^backend_' >> test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
