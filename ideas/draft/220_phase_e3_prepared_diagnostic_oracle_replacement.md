# 220 Phase E3 prepared diagnostic/oracle replacement

## Goal

Replace prepared printer/debug/oracle/string authority with route-native or
BIR-owned diagnostics only after equivalent semantic and fallback coverage is
available.

This phase exists because prepared diagnostics are still the comparison surface
that proves many route-first migrations are correct.

## Prerequisite

Do not open this draft until E0 names diagnostic/oracle rows that are ready for
replacement planning, and any required E1 semantic facts exist.

## Direction

Replace diagnostics one row or one tightly scoped diagnostic family at a time.
The replacement must preserve byte-stable output unless the semantic change is
explicitly justified and separately approved.

## In Scope

- Prepared printer rows that E0 classifies as route-native replacement
  candidates.
- Helper-oracle rows where route-native positive/negative/fallback evidence is
  complete.
- Route-debug rows with proven wrapper compatibility.
- Expected-string authority and baseline guardrails for the selected row.

## Out Of Scope

- Broad prepared printer or CLI dump replacement.
- Baseline refreshes, helper renames, unsupported downgrades, or output
  relabeling as progress.
- Removing prepared diagnostics before route-native equivalents exist.
- Target-wrapper or emission policy migration.

## Expected Output

The closure note must contain:

- the exact diagnostic/oracle/string row replaced or preserved;
- the route-native facts that now explain the row;
- positive, absent, invalid, duplicate/conflict, mismatch, and fallback proof;
- byte-stability or explicitly justified semantic-output proof;
- any remaining prepared diagnostic/oracle blockers.

## Reviewer Reject Signals

- Using production greenness as diagnostic replacement proof.
- Weakening or deleting the oracle that proves route/prepared equivalence.
- Combining multiple route families or broad dump sections into one slice.
