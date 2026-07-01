Status: Active
Source Idea Path: ideas/open/484_bir_semantic_local_address_provenance_array_element_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Disposition And Close Readiness

# Current Packet

## Just Finished

Completed Step 4 for idea 484 by recording residual disposition and handoff
readiness after Step 3 routed the producer authority blocker.

Lifecycle disposition:

| Question | Disposition |
| --- | --- |
| Is idea 484 complete as a producer implementation? | No. The required authority cannot be produced from current BIR/HIR route-local lowering data without raw-shape inference. |
| Is idea 484 close-ready as a negative/blocked producer result? | Yes. It defined the required contract and identified the exact lower-level prerequisite. |
| Can scalar local-load production resume? | No. It still lacks explicit pointer-to-local-element provenance and index/range authority. |
| Can RV64/MIR lowering proceed? | No. Target lowering would consume unproven address/provenance facts. |

Exact prerequisite follow-up:

`BIR local array address derivation and index-range authority carrier`

Follow-up scope:

- durable local array or local compound-literal source-object identity;
- explicit array-decay/local-address derivation identity and site;
- ordered element path from source object to consumer address;
- dynamic index identity and range proof with proof source plus
  path/dominance validity;
- element layout/range and scalar in-bounds status;
- pointer-to-local-element provenance status;
- unavailable statuses for missing source object, missing derivation,
  missing index/range, out-of-bounds, aggregate/member, integer-pointer
  round-trip, global, variadic/runtime, unsupported type, bootstrap,
  raw-shape-only, and target-only cases.

Residuals for idea 484 after that prerequisite:

- package the lower-level authority into the Step 2 local-address/array-element
  record shape;
- then hand back to scalar local-load producer work;
- keep RV64/MIR lowering and scalar-load consumption out of idea 484 until the
  authority records exist.

Artifact:

- `build/agent_state/484_step4_residual_disposition/disposition.md`

## Suggested Next

Ask the plan owner to close or split idea 484 as a blocked/negative producer
result and activate the prerequisite follow-up above if continuing this failure
family.

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
git diff --check
```

Result: passed.
