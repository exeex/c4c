Status: Active
Source Idea Path: ideas/open/486_bir_index_range_proof_path_dominance_carrier.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Populate Checker Inputs From Proof Facts

# Current Packet

## Just Finished

Implemented Step 5: production dynamic local-array checker inputs now populate
through `local_array_index_range_checker_inputs` only from matching
`local_array_proof_facts`. Available checker records are rebuilt through the
existing `evaluate_local_array_index_range_proof` checker from available proof
facts, and fail-closed statuses are preserved for missing/non-available proof
facts, selected-path-only or interval-only inference, unsupported boundary,
missing coordinate, clobber, alias/phi, unknown effect, non-covering path,
non-dominating/non-guarding proof, operand-role mismatch, and coordinate
confusion. Follow-up repair in this packet fixed the checker-input adapter so
`operand_roles_match_index` is derived from proof operands versus the element
path dynamic index instead of being forced true.

## Suggested Next

Execute Step 6 from `plan.md`: re-probe available and fail-closed checker
input representatives and record whether idea 484 packaging can resume.

## Watchouts

- Consume `local_array_proof_facts`; do not re-derive from
  `local_array_index_range_proofs`, selected paths, interval effects, endpoint
  bridges, final homes, raw testcase shape, or synthetic effect inputs.
- Idea 484 packaging, scalar loads, RV64/MIR lowering, and broad generic range
  analysis were not implemented in this packet.

## Proof

Delegated Step 5 proof:

```sh
cmake --build build -j2 > test_after.log 2>&1 && ctest --test-dir build -j2 --output-on-failure -R '^backend_' >> test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`. A final
`git diff --check` was appended after updating `todo.md` and the ignored
summary artifact.
