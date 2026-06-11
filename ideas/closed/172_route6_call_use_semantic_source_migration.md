# 172 Route 6 call-use semantic source migration

## Goal

Migrate one call argument or result semantic source consumer class to Route 6
BIR call-use source records while keeping ABI/layout records prepared-owned.

## Source

This idea comes from Phase C:

- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`
- Route 6 row under `## Step 4 Follow-Up Ideas And Proof Routes`

## Scope

- One selected call argument/result source class.
- Source-producer, direct-global, eligible memory/publication, result, or lane
  semantic source facts.
- Prepared call plans as ABI/layout and oracle surfaces during migration.

## Out Of Scope

- ABI register/stack placement, outgoing stack sizing, variadic FPR count,
  clobbers, byval lanes, scratch requirements, destination homes,
  helper/carrier protocols, aggregate transport layout, and broad call lowering
  rewrites.

## Acceptance Criteria

- The selected call consumer reads `Route6CallUseSourceIndex` for semantic
  source facts.
- ABI/layout-bound reads still come from prepared call plans.
- Oracle tests include source-producer, direct-global, eligible memory or
  publication, and ABI-bound negative cases for the selected class.

## Proof Route

Run call-source oracle tests for the selected class, then the narrow
call-lowering subset for that class. Escalate to broader backend tests if
public call APIs or plan projections change.

## Completion Notes

Closed after the selected AArch64 scalar call-argument source-producer consumer
migration runbook completed. The selected consumer reads indexed Route 6
call-use source records for semantic source-producer facts. ABI/layout policy
remains prepared-owned.

No prepared semantic-source surface was contracted during Step 5 because the
prepared lookup still serves as fallback/oracle coverage and still supports
recursive binary operand materialization where the call-argument Route 6 index
is intentionally absent.

Close proof used the canonical before/after logs with the delegated
non-decreasing regression policy:

```bash
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py \
  --before test_before.log \
  --after test_after.log \
  --allow-non-decreasing-passed
```

Result: PASS, 2/2 tests before and 2/2 tests after.

## Reviewer Reject Signals

- Copying call plans, ABI placement, aggregate transport, or helper/carrier
  policy into BIR.
- Switching broad call lowering in one slice.
- Weakening call-boundary expectations to claim cache contraction.
