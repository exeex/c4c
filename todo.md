Status: Active
Source Idea Path: ideas/open/489_bir_dynamic_local_array_proof_population_from_lir_coordinates.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Populate Proof Facts From Range Proof Certificates

# Current Packet

## Just Finished

Implemented Step 5: dynamic local-array proof facts now populate through
`local_array_proof_facts` only from matching production
`local_array_index_range_proofs` certificate records. The proof-fact producer
preserves fail-closed certificate statuses for missing/non-available
certificates, selected-path-only or interval-only inference, unsupported
boundary, missing coordinate, clobber, alias/phi, unknown effect,
non-covering path, non-dominating/non-guarding proof, and coordinate confusion.

## Suggested Next

Execute Step 6 from `plan.md`: re-probe available and fail-closed proof-fact
representatives and record whether idea 486 checker input population can
resume.

## Watchouts

- Do not re-derive selected-path coverage, interval no-clobber, endpoint
  bridges, final homes, raw testcase shape, or synthetic effect inputs.
- Idea 486 checker input population was not added in this packet; it remains
  the downstream handoff.
- Idea 484 packaging, scalar loads, and RV64/MIR lowering remain out of scope.

## Proof

Delegated Step 5 proof:

```sh
cmake --build build -j2 > test_after.log 2>&1 && ctest --test-dir build -j2 --output-on-failure -R '^backend_' >> test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`. A final
`git diff --check` was appended after updating `todo.md` and the ignored
summary artifact.
