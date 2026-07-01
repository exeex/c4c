Status: Active
Source Idea Path: ideas/open/485_bir_local_array_address_derivation_index_range_authority_carrier.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Local Array Carrier Inputs

# Current Packet

## Just Finished

Completed Step 1 for idea 485 by auditing local array carrier inputs for the
`pr38048-1.c` representative shape:

```text
decl mat: int[2][1]
decl a: int*[1] = mat#L0
det#L2 += a#L1[i#L3][0]
```

Carrier input classification:

| Fact family | Accepted current input | Missing lower-level fact |
| --- | --- | --- |
| Source object | static local-array alloca parsing creates element slots and `bir::LocalSlot` layout facts | durable source-object record with object id, declaration, rank, total range, lifetime/scope, and object-to-slot relation |
| Derivation/view | GEP/local pointer paths identify pointer/view result and route-local base slots | explicit array-decay/local-address derivation identity, source site, result pointer/view, and provenance-preserving status |
| Element path | immediate index checks and dynamic GEP helpers preserve some index operands | ordered semantic element-path record tied to source object and consumer |
| Index/range | HIR exposes `i#L3 < 2`; BIR GEP helpers carry lowered dynamic index value | dynamic range-proof carrier with lower/upper bounds, proof source, signedness/width, and path/dominance validity |
| Layout/range | scalar array alloca and layout helpers provide element type/count/size/stride seeds | durable element byte-range and scalar in-bounds proof tied to the source object |
| Provenance | `PointerAddress`/`MemoryAccessProvenance` can carry provenance payloads for some paths | local-array pointer-to-element provenance status tied to object, derivation, path, and consumer use |

Current local-memory lowering surfaces are mostly route-local aids:

- `LocalArraySlots`, `LocalPointerArrayBase`, `DynamicLocalPointerArrayAccess`,
  `DynamicLocalAggregateArrayAccess`, `LocalSlotAddress`, and
  `PointerAddress` carry useful slots, offsets, lowered index values, and type
  text, but they are not durable authority records;
- `lowered_function_.local_slots` is durable storage/layout output for
  individual slots, but it does not publish the source local array object and
  element relationship as authority;
- HIR/probe evidence exposes the representative loop range source, but current
  BIR carrier state does not prove it reaches the consumer access.

Step 2 readiness:

- Step 2 can define a bounded carrier contract with available/unavailable
  records for object identity, derivation, element path, range proof,
  layout/range, and provenance.
- Step 2 should preserve a likely Step 3 implementation blocker: dynamic
  index-range proof and path/dominance validity may still require new
  lower-level control-flow/range-proof production.

Artifact:

- `build/agent_state/485_step1_local_array_carrier_inputs/audit.md`

## Suggested Next

Execute Step 2: define the local array address derivation and index-range
carrier contract, including required available records and fail-closed
unavailable statuses.

## Watchouts

- Do not implement scalar local-load consumption or idea 484 packaging in this
  idea.
- Do not accept integer-pointer round-trip provenance.
- Do not infer authority from route-local `element_slots`, `base_index`,
  lowered index values, type text, testcase names, value names, dump order,
  final homes, or target fallback behavior.
- Treat `lowered_function_.local_slots` as storage/layout output, not by itself
  source-object or derivation authority.
- Keep dynamic index-range proof and path/dominance validity explicit; do not
  infer them from the `pr38048-1.c` loop shape unless a producer record says so.
- Keep global, aggregate/member, variadic/runtime, F128, complex/vector,
  volatile/atomic, bootstrap, raw-shape-only, and target-only cases
  fail-closed.
- Do not touch `review/`, canonical logs, baseline files, implementation files,
  or tests until an executor receives a bounded packet.

## Proof

Delegated proof:

```sh
git diff --check
```

Result: passed.
