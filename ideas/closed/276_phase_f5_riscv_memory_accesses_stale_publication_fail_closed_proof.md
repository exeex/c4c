# 276 Phase F5 RISC-V memory_accesses stale-publication fail-closed proof

## Goal

Use the supported stale-publication fixture from idea 275 to prove one bounded
RISC-V same-consumer `PreparedFunctionLookups::memory_accesses` stale public
row fails closed on the real backend path.

## Why This Exists

Idea 274 identified the needed stale-publication fail-closed proof but closed
as blocked because normal fixture construction could not express the stale row
without hand-built state. Idea 275 added supported fixture construction for a
stale public `memory_accesses` row. This idea returns to the original proof
obligation now that the fixture blocker is gone.

## In Scope

- Reuse the supported stale public `memory_accesses` fixture from idea 275.
- Exercise the real RISC-V same-consumer backend path, not helper-only
  formatting or classification.
- Prove the stale public memory row is rejected as Route 3 / Route 5 semantic
  agreement.
- Preserve the compatible `LoadLocal` path and exact `lw a1, 12(s2)` output.
- Record the fail-closed status/debug evidence needed to distinguish stale-row
  rejection from missing fixture support.

## Out of Scope

- Do not broaden this into cross-publication, exhaustive byte-offset coverage,
  x86 parity, edge-publication families, metadata, liveness, aggregate
  retirement, or draft 155 work.
- Do not introduce synthetic post-construction mutation as proof evidence.
- Do not demote, privatize, wrap, delete, or retire prepared aggregate APIs.

## Acceptance Criteria

- A narrow backend proof covers the supported stale-publication fixture and
  the compatible positive path.
- The stale public `memory_accesses` row fails closed without rendering an
  accepted instruction for the stale row.
- Positive output remains byte-stable at `lw a1, 12(s2)`.
- Status/debug evidence names the stale public row and the current Route 3 /
  Route 5 memory-source authority relation.
- Fallback, helper/oracle status, route-debug output, prepared-printer output,
  wrapper output, exact target output, and baseline behavior are not weakened.

## Reviewer Reject Signals

- Reject helper-only proofs claimed as backend same-consumer proof.
- Reject testcase-shaped shortcuts, named-case-only branches, or direct stale
  state mutation after fixture construction.
- Reject expectation rewrites, unsupported downgrades, weaker status or
  fallback contracts, route-debug weakening, prepared-printer weakening,
  wrapper weakening, baseline weakening, or exact-output relaxation without
  explicit user approval.
- Reject broad Route 3 / Route 5 rewrites, API privatization, prepared
  aggregate retirement, x86 parity work, edge-publication work, metadata,
  liveness, or draft 155 work under this idea.
- Reject a route that leaves the stale public row accepted behind a renamed
  helper or wrapper.

## Source

Follow-up to `ideas/closed/274_phase_f5_riscv_memory_accesses_stale_publication_fail_closed_proof.md`
and `ideas/closed/275_riscv_memory_accesses_stale_publication_fixture_support.md`.

## Closure Notes

Closed after the supported stale-publication fixture was driven through the
real RISC-V same-consumer backend path. The proof rejects the stale public
`memory_accesses` row while preserving the compatible dynamic stack-source
`LoadLocal` output at exactly `lw a1, 12(s2)`.

Close gate used the matching focused acceptance scope:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

That guard passed with 1/1 tests passing before and after, no new failures,
and no new tests over the 30 second threshold.
