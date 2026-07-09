# Mixed Local/Global Publication Authority

Status: Open
Type: Implementation
Parent: `ideas/closed/631_direct_global_symbol_local_memory_policy.md`
Related:
- `ideas/closed/631_direct_global_symbol_local_memory_policy.md`
- `ideas/closed/608_prepared_global_data_authority.md`
- `ideas/closed/600_pointer_value_memory_use_freshness_authority.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
Owning Layer: prepared publication authority for mixed local/global memory
Queue Order: 40
Prerequisites: local stores, global loads/stores, pointer freshness, and
publication ordering must be explicit before RV64 consumes mixed local/global
memory traffic
Proof Surface: rows with scalar global-memory facts whose remaining stop is
mixed ordinary local and pointer/global publication rather than direct
global-symbol local-memory

## Goal

Classify and publish the authority needed for mixed local/global memory
publication rows without broadening direct global-symbol local-memory policy.

## Why This Exists

Idea 631 Step 5 reclassified rows such as `src/pr57861.c`, `src/pr58431.c`,
`src/pr68185.c`, `src/pr68321.c`, and `src/pr70005.c`. These rows have scalar
global-memory facts, but their remaining stops involve mixed local stores,
global pointer traffic, freshness, or publication ordering. No refreshed row
still proved a direct `addr @symbol` local-memory consumer gap.

## In Scope

- Refresh the mixed local/global residual rows and identify the first missing
  publication, freshness, or ordering fact.
- Distinguish local publication, global publication, pointer freshness, and
  aggregate lanes before choosing a producer or consumer packet.
- Add prepared/prealloc producer facts or focused RV64 consumption only for a
  shared authority shape proven by current diagnostics.
- Preserve fail-closed diagnostics for stale pointer values, ambiguous
  publication order, missing local/global ownership, and aggregate-lane
  mismatches.

## Out Of Scope

- Direct global-symbol local-memory support closed by idea `631`.
- Prepared global value-location consumption owned by idea `621`.
- Aggregate stack-home policy owned by idea `633`.
- Move-bundle fan-in authority owned by idea `637`.
- Runtime/library policy, expectations, unsupported markers, allowlists,
  timeouts, or accounting.

## Acceptance Criteria

- A refreshed probe identifies whether each row is blocked by local
  publication, global publication, pointer freshness, aggregate ownership, or
  an RV64 consumer rule.
- At least one shared mixed-publication family gains explicit authority and
  moves past its current owner, or the route splits the rows into more precise
  source ideas.
- Negative proof keeps direct global-symbol local-memory, ordinary global
  memory, aggregate homes, and runtime-only failures outside this idea.

## Reviewer Reject Signals

- Reject claiming direct global-symbol local-memory progress for rows whose
  refreshed dumps show mixed local/global publication traffic.
- Reject testcase-shaped fixes for the named Step 5 rows.
- Reject using final assembly order, source statement order, or diagnostic text
  as publication authority.
- Reject expectation, unsupported-marker, allowlist, timeout, runtime, or
  accounting changes as capability progress.
- Reject helper renames or diagnostic wording changes that leave the same
  missing publication or freshness authority behind a new label.
