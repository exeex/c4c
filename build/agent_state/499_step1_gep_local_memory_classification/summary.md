# 499 Step 1 GEP Local-Memory Classification

Step 1 reproduced and classified the semantic `gep local-memory` admission
family selected by idea 496.

## Evidence Source

- Source row evidence:
  `build/agent_state/422_step1_semantic_producer_evidence/semantic_lir_to_bir_rows.tsv`
- Parent selection:
  `build/agent_state/496_step2_next_owner_selection/decision.md`
- Current scan logs referenced by each row:
  `build/rv64_gcc_c_torture_backend/*/case.log`
- Representative source snippets were read only from
  `tests/c/external/gcc_torture/src/`.

## Reproduced Row Count

The family contains `62` rows where `semantic_family` is
`gep local-memory semantic family`. The durable row list is
`rows.tsv`; classification counts are in `classification_counts.tsv`.

## Classification

| Classification | Count | Disposition |
| --- | ---: | --- |
| `direct_local_object_gep_candidate` | 3 | Available local-memory candidate shape for the next producer contract. |
| `global_or_static_object_gep_boundary` | 6 | Later owner; not part of the focused local-memory contract. |
| `pointer_or_formal_provenance_boundary` | 24 | Fail closed until provenance ties the base to a concrete source object/range. |
| `runtime_or_string_intrinsic_boundary` | 17 | Route to runtime/string intrinsic producer or fail closed before GEP admission. |
| `aggregate_member_flexible_or_alias_boundary` | 11 | Fail closed until aggregate/member/flexible-layout or alias provenance is explicit. |
| `variadic_boundary` | 1 | Variadic/byval routing is first owner. |

## Available Local-Memory Candidate Shapes

The next implementation packet should start with direct local-object GEP
admission, not with formal pointer inference or global/static object GEPs.
Candidate representatives:

- `src/pr80421.c`: local stack array `char c[]`, pointer `f = c + 390`, then
  dynamic `f[g]` access.
- `src/930614-2.c`: local multidimensional array `x[i][k][j][l]`.
- `src/pr24851.c`: local array with in-object negative subscript through
  `q = &a[1]` and `p = &q[-1]`.

These are candidates only when the producer can publish explicit local source
object, address derivation, element path, offset/layout, range, and coordinate
identity.

## Fail-Closed Boundary Shapes

The classifier keeps these distinct instead of treating the full 62-row family
as available:

- Formal or runtime pointer provenance, including `void *` byte-offset casts,
  negative indexing from a formal pointer, pointer/integer round trips, and
  global pointer variables.
- Global/static object GEPs such as `src/20000717-4.c`, `src/20031214-1.c`,
  `src/20051104-1.c`, `src/20080424-1.c`, `src/20120808-1.c`, and
  `src/ieee/copysign2.c`; these are later-owner/global-data work, not the
  focused local-memory contract.
- Runtime/string intrinsic harnesses such as `memcpy`, `memset`, `strcpy`,
  `strlen`, and `__builtin_prefetch`.
- Aggregate/member/flexible-layout and alias cases, including malloc-sized
  trailing storage, packed nested structures, field-to-container casts, and
  `may_alias`.
- Variadic aggregate argument routing.

## First Owner

First implementation owner: BIR semantic GEP local-memory producer contract for
direct local-object candidates only.

Step 2 should define the admission contract and fail-closed vocabulary before
code changes. If the contract discovers missing lower authority for the direct
candidate shape, route to that lower prerequisite instead of implementing
target or raw-shape inference.
