# 496 Step 2 Next Owner Selection

Step 2 selected the next owner from the Step 1 semantic-admission refresh.

## Inputs

- Step 1 summary:
  `build/agent_state/496_step1_semantic_admission_after_scalar_local_loads/summary.md`
- Step 1 counts:
  `build/agent_state/496_step1_semantic_admission_after_scalar_local_loads/semantic_family_counts.tsv`
- Step 1 representative rows:
  `build/agent_state/496_step1_semantic_admission_after_scalar_local_loads/semantic_family_representatives.tsv`
- Step 1 local-load disposition:
  `build/agent_state/496_step1_semantic_admission_after_scalar_local_loads/local_load_disposition.tsv`
- Open idea inventory:
  `ideas/open/*.md`

## Decision

Selected next owner: BIR semantic `gep local-memory` admission producer.

Selected next packet: create or activate a focused producer packet for the `62`
row `gep local-memory semantic family`, using representatives
`src/pr44468.c`, `src/pr48571-1.c`, `src/pr65956.c`, `src/pr80421.c`, and
`src/20000717-4.c`.

This is a producer packet, not an RV64/MIR consumer packet. The packet should
classify local-address/provenance authority needed for semantic GEP admission,
define fail-closed statuses for missing/incoherent authority, and only then
hand consumers forward.

## Why This Owner

The Step 1 family counts identify the largest remaining producer gap after the
mixed load-family disposition:

| Rank | Family | Count | Step 2 route |
| ---: | --- | ---: | --- |
| 1 | `gep local-memory semantic family` | 62 | selected producer owner |
| 2 | `store local-memory semantic family` | 58 | later write/store producer |
| 3 | `semantic call family 'direct-call semantic family'` | 52 | later call producer |
| 4 | `scalar/local-memory semantic family` | 49 | later local-memory split |
| 5 | `runtime/intrinsic family 'memcpy runtime family'` | 19 | later runtime producer |

The `load local-memory semantic family` remains `79` rows, but Step 1 classified
it as mixed after idea 483: clean local-array scalar-load consumers may now use
`local_array_scalar_local_loads`, while aggregate/member, variadic, global,
pointer-derived, and integer-pointer-round-trip rows remain producer-owned
boundaries. That mixed state is not the next broad producer owner.

## Open Idea Inventory Result

No existing open idea is a clean, unblocked match for semantic GEP local-memory
admission.

`ideas/open/442_pointer_value_memory_provenance_publication.md` is related to
pointer provenance vocabulary, but it is explicitly parked on missing
closed-world/internal/private/no-external-caller authority and is scoped around
runtime pointer dereferences such as `930930-1`. It should not be used as a
semantic GEP local-memory admission packet without plan-owner review.

Therefore lifecycle should not switch to an existing more-specific open idea in
the current inventory. The recommended next lifecycle action is for the
supervisor/plan-owner to create or activate a focused semantic GEP local-memory
producer idea, or to expand the active 496 runbook through the plan owner if
that is the chosen lifecycle route.

## Producer vs Consumer Boundary

Valid consumers of `local_array_scalar_local_loads` may proceed only when they
read matching available scalar-load facts. They must not reconstruct local
object, element offset, layout, range, provenance, exact-address checks, or
scalar-load identity from lower surfaces, final homes, raw testcase shape,
names, or target behavior.

The selected GEP route is not such a consumer packet. It is a producer authority
packet for semantic local-address/GEP admission. If implementation discovers
that a representative already has all required producer facts, only then should
it hand off a fact-consuming downstream packet.
