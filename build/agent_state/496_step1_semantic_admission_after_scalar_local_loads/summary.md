# 496 Step 1 Semantic Admission Refresh

Step 1 refreshed the semantic `lir_to_bir` admission classification after idea
483 published `local_array_scalar_local_loads`.

## Evidence Sources

- Fresh whole-scan summary:
  `build/agent_state/rv64_gcc_c_torture_backend_summary.full.tsv`
- Fresh bucket map:
  `docs/rv64_gcc_torture_post_contract/failure_bucket_map.md`
- Prior semantic producer rows:
  `build/agent_state/422_step1_semantic_producer_evidence/semantic_lir_to_bir_rows.tsv`
- Corrected local-load audit:
  `build/agent_state/483_step1_corrected_local_load_search/audit.md`
- Published scalar-load fact surface:
  `build/agent_state/483_step2_scalar_local_loads_from_provenance/summary.md`
- Residual disposition:
  `build/agent_state/483_step3_residual_disposition_after_scalar_local_loads/disposition.md`

## Refreshed Counts

The fresh full RV64 gcc_torture backend scan records `1467` total cases, `314`
passing cases, and `1153` failing cases. The semantic `lir_to_bir` admission
bucket remains `373` rows.

The refreshed disjoint semantic-family counts are recorded in
`semantic_family_counts.tsv`; they sum to `373`:

- `79` load local-memory
- `62` gep local-memory
- `58` store local-memory
- `52` direct-call
- `49` scalar/local-memory
- `19` memcpy runtime
- `16` alloca local-memory
- `15` memset runtime
- `10` scalar-control-flow
- `9` function-signature
- `3` call-return
- `1` scalar-binop

## Local-Array Scalar-Load Disposition

All `79` rows from the corrected idea-483 load local-memory semantic family are
still present inside the fresh `373` semantic bucket. The classification change
is not that the whole family is solved; it is that clean local-array
element-load consumers now have a production authority:
`local_array_scalar_local_loads`.

Rows with clean available local-array scalar-load facts should now be treated
as fact-consuming downstream work. Representative coverage includes
`src/pr38048-1.c` and the non-round-trip local-array portions of
`src/pr22098-1.c`, `src/pr22098-2.c`, and `src/pr22098-3.c`. Consumers must use
the published fact and must not re-derive local object, element offset, layout,
range, provenance, exact-address, or scalar-load identity from lower surfaces or
target behavior.

The remaining load-family representatives are not automatically consumer-ready:
`src/multi-ix.c` is variadic/va_arg contaminated in its latest failing function;
`src/20041124-1.c`, `src/991228-1.c`, `src/cbrt.c`, `src/pr15296.c`, and
`src/20050826-2.c` are aggregate/member/union/object-representation boundaries;
`src/pr15262.c`, `src/pr33870-1.c`, and related pointer rows remain
pointer/provenance or integer-pointer-round-trip boundaries when no available
scalar-load fact exists.

## Remaining First-Owner Families

The next packet should choose one first owner rather than starting RV64 lowering
from the whole semantic bucket. Highest remaining producer families by row count
are:

- `gep local-memory semantic family`: `62`
- `store local-memory semantic family`: `58`
- `semantic call family 'direct-call semantic family'`: `52`
- `scalar/local-memory semantic family`: `49`
- `runtime/intrinsic family 'memcpy runtime family'`: `19`
- `alloca local-memory semantic family`: `16`
- `runtime/intrinsic family 'memset runtime family'`: `15`

Recommended Step 2 decision: select a producer-owned packet from these families
or explicitly hand off a fact-consuming scalar local-load consumer that reads
`local_array_scalar_local_loads`.
