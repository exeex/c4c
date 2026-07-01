Status: Active
Source Idea Path: ideas/open/483_bir_semantic_local_memory_scalar_load_producer.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Focused Local-Load Representative Evidence

# Current Packet

## Just Finished

Continued Step 1 for idea 483 by searching the full 79-row local-memory load
bucket for a non-pointer/provenance ordinary scalar local-object load
representative after reviewer rejection of `pr22098-1.c`.

Evidence generated:

- `build/agent_state/483_step1_corrected_local_load_search/local_load_rows.tsv`
  lists all 79 bucket rows;
- `build/agent_state/483_step1_corrected_local_load_search/probes/*.dump_hir.*`
  contains fresh HIR probes for all 79 rows;
- `build/agent_state/483_step1_corrected_local_load_search/hir_direct_local_array_candidates.tsv`
  records direct local-array-looking candidates;
- `build/agent_state/483_step1_corrected_local_load_search/hir_plain_local_rhs_candidates.tsv`
  records broader plain local RHS candidates.

Search result:

- no non-pointer/provenance ordinary scalar local-object load representative was
  found in the 79-row bucket;
- the apparent scalar candidates all require a rejected prerequisite:
  pointer/provenance, array-decay/GEP, aggregate/member, union/object
  representation, variadic/va_arg, global source, or runtime/call
  contamination.

Representative rejected candidates:

| Candidate | Observed shape | Rejection |
| --- | --- | --- |
| `src/pr22098-{1,2,3}.c` | compound literal array element through pointer or integer-pointer round-trip | Pointer/provenance or integer-pointer authority required. |
| `src/multi-ix.c` | `a0#L1[0] -> i0#L41` style local array loads | Variadic/va_arg contaminated; latest failure is `s`, not the direct load site. |
| `src/pr38048-1.c` | `det += a#L1[i#L3][0]` | Uses local pointer `a = mat`; needs array-decay/address/provenance facts. |
| `src/20050826-2.c` | `rt#L2[1] = rt#L2[0]` | Aggregate struct array copy plus pointer-array/call contamination. |
| `src/pr33870-1.c`, `src/pr33870.c` | `p#L3 = a#L2[0]` | Pointer value loaded from local array; pointer-list/struct-member merge route. |
| `src/pr15262.c`, `src/pr15296.c` | pointer-parameter struct/union member loads | Aggregate/member and alias/provenance boundary. |

Corrected blocker/prerequisite:

- Step 2 should not define a scalar local-load implementation contract from
  the current 79-row evidence;
- the prerequisite first owner is `BIR semantic local-address/provenance and
  array-element access authority`;
- that owner must expose local object, array decay, index/offset, element
  layout, and pointer-to-local-element provenance facts before idea 483 can
  safely consume these rows.

Artifact:

- `build/agent_state/483_step1_corrected_local_load_search/audit.md`

## Suggested Next

Route the next packet to lifecycle/plan-owner or a new source idea for `BIR
semantic local-address/provenance and array-element access authority`. Do not
advance idea 483 to a scalar local-load implementation contract until that
prerequisite is available.

## Watchouts

- Do not proceed to Step 2 with any pointer/provenance, GEP, aggregate/member,
  byval/va_arg, volatile/atomic, complex, vector, F128, bootstrap, or target
  fallback shape as the selected scalar local-load target under idea 483.
- Do not repair missing BIR semantic facts in RV64/MIR.
- Do not infer producer facts from testcase names, value names, raw dump order,
  final homes, or target fallback behavior.
- Keep pointer/provenance and array-decay/GEP loads fail-closed unless a
  separate source idea explicitly owns the prerequisite facts.
- Do not touch `review/`, canonical logs, baseline files, implementation files,
  or tests.

## Proof

Delegated proof:

```sh
git diff --check
```

Result: passed.
