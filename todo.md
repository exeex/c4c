Status: Active
Source Idea Path: ideas/open/485_bir_local_array_address_derivation_index_range_authority_carrier.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Address Derivation And Index-Range Carrier Contract

# Current Packet

## Just Finished

Completed Step 2 for idea 485 by defining the durable local array address
derivation and index-range carrier contract for the `pr38048-1.c`
representative shape:

```text
decl mat: int[2][1]
decl a: int*[1] = mat#L0
det#L2 += a#L1[i#L3][0]
```

Durable carrier shape:

| Record family | Required fields | Fail-closed boundary |
| --- | --- | --- |
| Source object | function, source object id/value id, declaration/materialization site, storage kind, type text, rank, element types/counts, total range, alignment, object-to-slot relation, lifetime/scope where available | missing/non-local source object, missing layout, missing object-to-slot relation |
| Address derivation | source object id, derivation kind, derivation site, derived pointer/view id, base view id when continuing from a view, constant base index, provenance-preserving status, cast status | missing derivation, unproven local derivation, missing pointer identity, integer-pointer round trip |
| Ordered element path | source object id, derivation id, consumer access id, ordered indices by rank, constant/dynamic index ids, index width/signedness, rank reached, static offset expression, selected slots where available | missing element path, missing index identity, raw slot-order inference |
| Index-range proof | index id, proof source, lower/upper bounds, predicate/operand ids, path/dominance validity, index clobber status, consumer access id | missing range proof, proof not dominating consumer, path not covering consumer, index clobbered |
| Layout/range | final element type, size, alignment, byte offset/range, object byte range, in-bounds status, scalar-vs-aggregate status | out-of-bounds, element not scalar, layout/range mismatch |
| Provenance | source object id, derivation id, element path id, consumer access id, carrying pointer/view id, available/unavailable status, crossed-boundary status | unknown provenance, global/runtime/variadic/aggregate/unsupported/raw/target-only boundaries |

Required unavailable statuses include:

- `missing_source_object`, `source_object_not_local`,
  `missing_source_object_layout`, `missing_object_to_slot_relation`,
  `missing_derivation`, `derivation_not_proven_local`,
  `missing_derived_pointer_identity`, `missing_element_path`,
  `missing_index_identity`, `missing_index_range_proof`,
  `range_proof_not_dominating_consumer`,
  `range_proof_path_not_covering_consumer`,
  `index_value_clobbered_before_consumer`, `element_out_of_bounds`,
  `element_not_scalar`, `layout_range_mismatch`, `unknown_provenance`,
  `integer_pointer_round_trip`, `global_source_object`,
  `aggregate_or_member_boundary`, `union_or_object_representation_boundary`,
  `variadic_or_va_arg_boundary`, `runtime_or_call_boundary`,
  `f128_complex_vector_or_volatile_atomic_boundary`, `bootstrap_boundary`,
  `raw_shape_only`, and `target_only_or_final_home_only`.

Step 3 readiness:

- A bounded first implementation packet exists for source-object,
  derivation, element-path, and layout/range record/status publication from
  static local-array alloca and explicit local array/view GEP paths.
- The full dynamic `pr38048-1.c` available status likely remains blocked until
  a lower-level index range proof and path/dominance carrier exists; Step 3
  should preserve `missing_index_range_proof` instead of inferring from loop
  shape.

Artifact:

- `build/agent_state/485_step2_address_derivation_index_range_contract/contract.md`

## Suggested Next

Execute Step 3: implement or route the first carrier producer packet. Keep the
first code packet bounded to source-object/derivation/element-path/layout
records with explicit unavailable statuses unless range-proof/path-dominance
authority is already available through producer data.

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
- If Step 3 cannot add source-object/derivation records independently, route to
  `BIR index range proof and path-dominance carrier` or the exact lower
  producer surface rather than adding partial raw-shape authority.
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
