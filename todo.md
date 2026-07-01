Status: Active
Source Idea Path: ideas/open/499_semantic_gep_local_memory_admission_producer.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define The Semantic GEP Admission Contract

# Current Packet

## Just Finished

Step 2 defined the narrow BIR semantic GEP local-memory admission contract for
direct local-object candidates. The contract in
`build/agent_state/499_step2_gep_local_memory_contract/` names the required
source object, derivation/provenance, element path, layout/range, coordinate
authority fields, available status, unavailable statuses, and Step 3
implementation disposition.

## Suggested Next

Execute Step 3 by implementing a production BIR semantic GEP local-memory
admission record populated only from matching available
`local_array_local_address_provenances` for local-object `AddressDerivation`
paths. Cover `src/pr80421.c`, `src/930614-2.c`, and `src/pr24851.c` candidate
shapes plus fail-closed representatives for global/static object GEPs,
pointer/formal provenance, runtime/string intrinsic, aggregate/member,
flexible-layout, alias, variadic, raw-shape-only, target-only, and coordinate
confusion.

## Watchouts

- This is a BIR semantic producer packet, not RV64/MIR lowering.
- Do not infer GEP authority from raw shape, names, final homes, lowered target
  behavior, or route-local slots.
- Step 3 can implement without routing a lower prerequisite, but only by
  consuming matching production `LocalArrayLocalAddressProvenanceRecord`
  authority. Missing or non-available lower records must remain fail-closed.
- Step 1 found only 3 direct local-object candidate rows. The 6 global/static
  object GEP rows are separated as later-owner boundaries, and the other 53 rows
  remain fail-closed or prerequisite-owned until explicit producer authority
  exists.
- Keep store local-memory, direct-call, scalar/local-memory, runtime,
  move-bundle, F128, stack-frame, and scalar-load work out of this packet.
- If a lower producer prerequisite is missing, route lifecycle to that
  prerequisite instead of implementing target inference.

## Proof

Step 2 contract proof:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_before.log --allow-non-decreasing-passed > test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
