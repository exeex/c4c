Status: Active
Source Idea Path: ideas/open/484_bir_semantic_local_address_provenance_array_element_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route Authority Producer

# Current Packet

## Just Finished

Completed Step 3 for idea 484 as a routed blocker, not an implementation.
Targeted inspection found that the current BIR/HIR producer data is
route-local lowering state and cannot publish the Step 2 authority record
without raw-shape inference.

Step 3 decision:

| Decision | Evidence | First missing authority |
| --- | --- | --- |
| Route, no implementation | `LocalArraySlots`, `LocalPointerArrayBase`, `DynamicLocalPointerArrayAccess`, `DynamicLocalAggregateArrayAccess`, and `PointerAddress` carry route-local slots, lowered index values, offsets, and type text only. | Lower-level semantic local-array address derivation and index-range authority carrier. |

Missing facts for an accepted producer:

- stable local object identity tied to declaration/storage, object rank,
  layout, range, and lifetime;
- explicit array-decay/local-address derivation identity and source site;
- ordered element path from the source object to the consumer address;
- dynamic index range proof with proof source and path/dominance validity;
- element layout/range and scalar in-bounds proof;
- pointer-to-local-element provenance status tied to source object, derived
  pointer/view, element path, and consumer use;
- durable unavailable statuses from the Step 2 contract.

Rejected Step 3 implementation approach:

- do not infer authority from route-local `element_slots`, `base_index`,
  lowered index values, type text, raw `pr38048-1.c` expression shape, value
  spellings, dump order, final homes, or RV64 fallback behavior.

Artifact:

- `build/agent_state/484_step3_local_address_authority_producer/blocker.md`

## Suggested Next

Execute Step 4 residual disposition for idea 484. Recommended lifecycle route:
split or activate the lower-level producer packet named `local array address
derivation and index-range authority carrier`; resume idea 484 only after that
surface can publish explicit object, derivation, element-path, index-range,
layout/range, provenance, and unavailable-status records.

## Watchouts

- Do not consume scalar local loads or repair RV64 lowering in idea 484.
- Do not accept integer-pointer round-trip provenance, including the
  `pr22098-1.c` `uintptr_t` path.
- Do not use `multi-ix.c` as the first driver; it is useful boundary evidence
  but contaminated by variadic/va_arg/memset paths.
- Do not infer local-address authority from testcase names, value names, raw
  dump order, final homes, or RV64 target fallback behavior.
- Keep global, aggregate/member, union/object-representation, variadic/va_arg,
  runtime/call, F128, complex, vector, volatile/atomic, bootstrap, and raw
  inference shapes fail-closed.
- Do not touch `review/`, baseline files, implementation files, tests, scalar
  load consumption, RV64/MIR lowering, expectation files, or allowlists.

## Proof

Delegated proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed.
