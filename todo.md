Status: Active
Source Idea Path: ideas/open/442_pointer_value_memory_provenance_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Re-Audit Pointer-Value Provenance Publication

# Current Packet

## Just Finished

Activated `ideas/open/442_pointer_value_memory_provenance_publication.md` as
the next active plan. Selection rationale: it is the direct concrete follow-up
from closed idea 438, after the pointer-value memory authority contract was
covered but `930930-1` remained fail-closed with unknown layout and
unknown-compatible range.

## Suggested Next

Execute Step 1: re-audit the four `930930-1` pointer-value memory accesses
against `prepare::prepared_pointer_value_memory_has_proven_authority` and
classify each as concrete-provenance candidate, opaque-compatibility candidate,
or intentionally unsupported.

## Watchouts

- Do not weaken `prepare::prepared_pointer_value_memory_has_proven_authority`.
- Do not infer pointer-value memory provenance, layout, object extent, or range
  in RV64 from raw pointer values, integer casts, filenames, function names,
  value names, object labels, or dump shape.
- Treat `layout_authority=unknown` and `range_verdict=UnknownCompatible` as
  fail-closed unless a named and tested opaque-compatibility policy is created.
- Keep store-source/global-memory publication, direct-global
  return/select-chain, and terminator/select publication out of this plan.

## Proof

Activation-only lifecycle change. Required local check:

```sh
git diff --check
```
