# Pointer/Global Local Publication Authority

Status: Closed
Type: Implementation
Parent: `ideas/closed/640_mixed_local_global_publication_authority.md`
Related:
- `ideas/closed/640_mixed_local_global_publication_authority.md`
- `ideas/closed/600_pointer_value_memory_use_freshness_authority.md`
- `ideas/closed/631_direct_global_symbol_local_memory_policy.md`
Owning Layer: prepared publication authority for pointer values stored through
ordinary local frame slots
Queue Order: 49
Prerequisites: pointer value freshness, local-slot identity, global object
identity, and publication order must be explicit before RV64 consumes a
pointer-valued local frame-slot access.
Proof Surface: `src/pr57861.c` after scalar frame-slot local-memory
publication has been separated from pointer-to-global local publication.

## Completion Note

Closed after the RV64 object route gained a narrow consumer for explicit
pointer/global local-publication authority. The implementation consumes an
available prepared `StoreLocalPublication` plus same-instruction direct-global
address materialization to store the selected global address into the exact
ordinary local frame slot, without treating scalar frame-slot facts or generic
direct-global local-memory policy as pointer publication authority.

Focused positive coverage proves the direct global-address publication through
a local pointer slot. Focused negative coverage keeps live reloads of that
local pointer publication fail-closed when the reloaded value is used as a later
memory-address base. The representative `pr57861.c` RV64 object route now emits
and disassembles through the `%lv.l` publication site. A local string-label
compound regression found during Step 5 was repaired by restoring only
anonymous same-instruction local-memory access lookup, preserving exact named
store lookup for pointer/global publication.

Close proof used the matched RV64 regression guard:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log
```

Result: PASS for matched
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "riscv64"`
logs, with before passed=89 failed=18 total=107, after passed=92 failed=18
total=110, delta passed=+3 failed=0, and new failing tests=0.

Reviewer report `review/idea649_review.md` found no blocking issues and judged
the route aligned with the source idea.

## Goal

Define and repair the explicit authority needed when a pointer value derived
from a global object is stored in, loaded from, or otherwise published through
an ordinary local frame slot.

## Why This Exists

Idea 640 repaired the scalar frame-slot local-memory lookup family and proved
`src/pr58431.c` can pass without broadening direct global-symbol local-memory
policy. The remaining `src/pr57861.c` evidence includes a pointer-to-global
local path such as `short *l = &f`, which is a pointer/global local
publication problem rather than scalar local-memory lookup.

## In Scope

- Refresh `src/pr57861.c` and nearby pointer-valued local-slot residuals after
  idea 640.
- Identify the prepared pointer value, global object source, local-slot
  destination, load/use point, and publication/freshness authority.
- Add producer or RV64 consumer support only when pointer freshness and local
  publication authority are explicit for the selected family.
- Preserve fail-closed diagnostics for stale pointer values, missing global
  object identity, ambiguous publication order, mismatched slots, and scalar
  local-memory-only facts.

## Out Of Scope

- Scalar frame-slot local-memory lookup repaired by idea `640`.
- Direct global-symbol local-memory support closed by idea `631`.
- Generic pointer freshness already closed by idea `600` unless this route
  exposes a new local-publication boundary.
- Aggregate/global-object materialization, stack-home aggregate policy,
  move-bundle fan-in authority, ABI/runtime policy, expectations,
  unsupported markers, allowlists, timeouts, or accounting.

## Acceptance Criteria

- A refreshed probe proves the first owner is pointer/global local publication
  or splits the row into a more precise owner.
- At least one complete-authority pointer-valued local-slot shape moves past
  its current owner, or the route records the exact missing producer or RV64
  consumer authority that blocks it.
- Negative proof keeps scalar-only frame-slot facts, direct global-symbol
  local memory, stale pointer values, and ambiguous local publication
  fail-closed.

## Reviewer Reject Signals

- Reject named-case fixes for `src/pr57861.c`, `l`, or `f` without a semantic
  pointer/global local-publication rule.
- Reject treating pointer-valued local slots as scalar frame-slot local-memory
  accesses repaired by idea `640`.
- Reject inferring pointer freshness or publication authority from source
  spelling, final assembly order, diagnostics, or testcase identity.
- Reject expectation, unsupported-marker, allowlist, timeout, runtime, or
  accounting changes as capability progress.
- Reject helper renames or diagnostic wording changes that leave the same
  missing pointer/global local-publication authority behind a new label.
