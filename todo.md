Status: Active
Source Idea Path: ideas/open/484_bir_semantic_local_address_provenance_array_element_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Local-Address Provenance Contract

# Current Packet

## Just Finished

Completed Step 2 for idea 484 by defining the bounded local-address and
array-element provenance contract around the `pr38048-1.c`
`a#L1[i#L3][0]` shape.

Accepted evidence:

| Case | Accepted shape | Contract boundary |
| --- | --- | --- |
| `src/pr38048-1.c` | `decl mat: int[2][1]`; `decl a: int*[1] = mat#L0`; `det#L2 += a#L1[i#L3][0]` | Producer-authority only: prove local object identity, array decay/local-address derivation, index/range, element layout/range, and pointer-to-local-element provenance. |

Required record fields:

- function identity, source derivation identity, consumer access identity,
  local object value id, derived pointer/view value id, and element access
  identity;
- source object type/rank, element type, size, alignment, total byte range,
  storage kind, and lifetime/scope identity where available;
- derivation kind (`array_decay`, `address_of_element`, or
  `direct_local_array_element`), source object id, derived pointer/view id,
  derivation site, and provenance-preserving status;
- ordered element path, index value ids/constants, index width/signedness,
  dynamic range proof source, and status for missing range;
- final element type, size, alignment, byte offset/range, in-bounds status, and
  scalar-vs-aggregate classification;
- provenance status tying the consumer address back to the local object and
  element path.

Fail-closed statuses required for Step 3:

- `missing_source_object`
- `source_object_not_local`
- `missing_array_decay_or_address_derivation`
- `missing_derived_pointer_home`
- `missing_index_identity`
- `missing_index_range`
- `element_out_of_bounds`
- `element_not_scalar`
- `unknown_or_mismatched_layout`
- `unknown_provenance`
- `integer_pointer_round_trip`
- `global_source_object`
- `aggregate_or_member_boundary`
- `union_or_object_representation_boundary`
- `variadic_or_va_arg_boundary`
- `runtime_or_call_boundary`
- `f128_complex_vector_or_volatile_atomic_boundary`
- `bootstrap_boundary`
- `raw_shape_only`
- `rv64_or_final_home_only`

Step 3 readiness:

- a bounded producer packet exists only if the current BIR/HIR semantic
  producer surface can carry local object identity, array-decay/local-address
  derivation identity, element path, index identities/ranges, element
  layout/range, provenance-preserving status, and unavailable statuses;
- if dynamic index range proof or derivation identity cannot be exposed without
  raw-shape inference, Step 3 must route to that lower-level producer surface
  instead of implementing partial authority.

Artifact:

- `build/agent_state/484_step2_local_address_provenance_contract/contract.md`

## Suggested Next

Execute Step 3: implement or route the bounded authority producer. Implement
only if the semantic producer data can explicitly carry the required
derivation, index/range, layout/range, and provenance facts; otherwise record
the exact lower-level blocker.

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
- Do not touch `review/`, canonical logs, baseline files, implementation files,
  or tests.

## Proof

Delegated proof:

```sh
git diff --check
```

Result: passed.
