# 239 Phase E5 PreparedBirModule demotion or retirement gate

## Goal

Decide whether the current E0 through E4 evidence is sufficient to rewrite,
supersede, keep blocked, or eventually open draft 155, and whether any
selected `PreparedBirModule` or `PreparedFunctionLookups` field group is ready
for demotion from durable public prepared artifact to private pass context or
target-policy product.

This is Phase E5 of the BIR/prealloc thinning program. This activation should
be treated as a queueable readiness gate, not as permission to delete
`PreparedBirModule`. It may rewrite or supersede draft 155 only if the
evidence says the prerequisites are complete.

## Current Readiness State

E0, E1, E2, E3, and E4 readiness analysis have closed, and many selected
semantic, private-pass-context, diagnostic/oracle, and x86 wrapper-adjacent
follow-ups have closed.

However, E4 left one accepted follow-up still open:

- `ideas/open/238_phase_e4_x86_route6_scalar_i32_route_debug_consumedplans_compatibility_follow_up.md`

E5 must treat that open idea as an explicit readiness blocker unless it has
closed before E5 activation. If 238 is still open when E5 runs, E5 may still
produce a gate artifact, but it must not claim that cross-target wrapper
convergence prerequisites are complete.

## Prerequisite

Before claiming any demotion or retirement readiness, E5 must verify:

- E0 has closed with a field-by-field ownership map;
- accepted E1 semantic duplicate migrations have closed;
- accepted E2 public lookup/API contractions have closed;
- required E3 diagnostic/oracle replacements have closed;
- E4 readiness has closed and any accepted E4 implementation follow-ups,
  including 238, are either closed or explicitly recorded as blockers;
- accepted baseline and string-authority state is non-regressive.

## Direction

E5 is still a gate before deletion. It must decide which outcome is safe:

- keep `PreparedBirModule` unchanged because blockers remain;
- demote selected fields to private pass context;
- split target-policy products out of the aggregate;
- rewrite draft 155 with precise deletion/demotion packets;
- open final implementation ideas for one field group at a time.

The default expected outcome is conservative: identify what is still blocked
and what exact field group could become a later final packet. E5 must not turn
selected route-reader, diagnostic-row, private-pass-context, or x86-only
wrapper evidence into whole-module retirement.

## In Scope

- Reading E0 through E4 durable artifacts and closure notes.
- Reading all E1/E2/E3/E4 follow-up closure notes, and explicitly checking
  whether 238 remains open.
- `PreparedBirModule` construction and mutation sites.
- `PreparedFunctionLookups` construction and public lookup-group delivery.
- MIR/codegen, printer/debug, target-wrapper, oracle, and test consumers.
- Compatibility adapters needed during demotion.
- Final proof strategy for any deletion or demotion route.
- Whether draft 155 should be rewritten, superseded, opened later, or kept
  blocked.

## Out Of Scope

- Direct broad deletion of `PreparedBirModule`.
- Direct broad deletion of `PreparedFunctionLookups`.
- Rename-only facade work.
- Combining retirement with unrelated backend feature work.
- Removing target policy, diagnostics, or fallback without proven replacements.
- Treating open idea 238 as completed evidence.
- Claiming riscv readiness from x86-only evidence.
- Weakening baselines, expected strings, helper-oracle names, supported-path
  status, printer/debug output, wrapper output, or fallback behavior.

## Expected Output

The closure note must contain:

- whether draft 155 should be rewritten, opened, superseded, or kept blocked;
- whether open idea 238 was closed before E5 ran or remains a blocker;
- the field groups ready for demotion or retirement;
- the field groups that must remain target/prepared policy or pass context;
- the field groups that must remain public fallback/oracle, diagnostic/string,
  or cross-target compatibility surfaces;
- compatibility adapters required during transition;
- a final proof strategy including full-suite/baseline expectations;
- follow-up implementation ideas for each safe final packet.

## Reviewer Reject Signals

- Claiming whole-module retirement from selected route-reader evidence.
- Hiding the second IR behind a renamed wrapper.
- Losing prepared printer/dump visibility needed for regression diagnosis.
- Deleting public consumers before cross-target, diagnostic, fallback, and
  baseline proof is complete.
- Treating x86-only Route 6 evidence as route-wide x86, riscv, or
  cross-target readiness.
- Treating 238 as closed if it is still open.
- Opening draft 155 without complete ownership, fallback/oracle, diagnostic,
  wrapper, baseline, and public-consumer proof.

## Closure Note

Closed by the Phase E5 gate.

The active runbook completed all five steps:

- the E0 through E4 prerequisite inventory was checked;
- `PreparedBirModule` and `PreparedFunctionLookups` field groups were mapped;
- the durable E5 gate artifact was written at
  `docs/bir_prealloc_fusion/phase_e5_prepared_bir_module_demotion_or_retirement_gate.md`;
- narrow successor ideas 240, 241, and 242 were opened for the only safe
  follow-up implementation shapes;
- closure validation confirmed this gate touched only docs and lifecycle
  artifacts, not implementation, expectation, baseline, or draft 155 files.

Closure disposition:

- draft 155 remains blocked and must not be opened as broad aggregate
  retirement work;
- idea 238 is closed and prerequisite-complete only for x86 Route 6 scalar
  `i32` route-debug / `ConsumedPlans` compatibility;
- no `PreparedBirModule` field group is ready for whole-field deletion,
  whole-field privatization, or aggregate retirement;
- no `PreparedFunctionLookups` group is ready for whole-group deletion,
  whole-group privatization, or aggregate replacement;
- target policy, private pass context, public fallback/oracle,
  diagnostic/string authority, wrappers, route-debug output, expected strings,
  and baseline behavior remain retained until future narrow proof says
  otherwise.

Close-time regression guard passed using canonical `test_before.log` and
`test_after.log` with `--allow-non-decreasing-passed`. This non-decreasing mode
was used because the close is docs/lifecycle-only maintenance with no
implementation changes, so equal pass counts are acceptable evidence when no
new failing tests appear:

- before: 2/2 passed;
- after: 2/2 passed;
- result: PASS, with no new failing tests.

The gate also records the accepted full-suite baseline
`log/baseline_8cebab4beba219e6a8cdef998bc970c8658ce28b.log`, which reports
3428/3428 passing at commit `8cebab4beba219e6a8cdef998bc970c8658ce28b`.
