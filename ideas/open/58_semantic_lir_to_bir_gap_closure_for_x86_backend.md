# Semantic LIR-To-BIR Gap Closure For X86 Backend

Status: Open
Created: 2026-04-20
Last-Updated: 2026-04-20
Parent Idea: [57_x86_backend_c_testsuite_capability_families.md](/workspaces/c4c/ideas/open/57_x86_backend_c_testsuite_capability_families.md)

## Intent

Close the upstream `semantic lir_to_bir` lowering gaps that currently stop
backend c-testsuite cases before the x86 prepared-emitter route can even
consume canonical contracts.

This idea exists because current backend failures are not all x86 emitter
problems. A large portion still dies earlier with:

`error: x86 backend emit path requires semantic lir_to_bir lowering before ...`

Until this bucket shrinks, downstream "prepared x86" completion claims remain
misleading.

## Owned Failure Family

This idea owns the current failure cluster with the diagnostic family:

`error: x86 backend emit path requires semantic lir_to_bir lowering before ...`

Current cluster size from the 2026-04-20 backend run: 41 failures.

## Current Known Failed Cases It Owns

Representative named failures already observed in this cluster:

- `backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`
- `backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`
- `c_testsuite_x86_backend_src_00037_c`
- `c_testsuite_x86_backend_src_00050_c`
- `c_testsuite_x86_backend_src_00051_c`
- `c_testsuite_x86_backend_src_00089_c`
- `c_testsuite_x86_backend_src_00095_c`
- `c_testsuite_x86_backend_src_00204_c`
- `c_testsuite_x86_backend_src_00207_c`
- `c_testsuite_x86_backend_src_00217_c`

This idea also owns any other current backend c-testsuite failures that report
the same diagnostic family and are not more precisely split into the dedicated
stack/addressing leaf in idea 62.

## Routing Checkpoint

The current lifecycle checkpoint after the bootstrap-global lowering repair is:

- `c_testsuite_x86_backend_src_00204_c` now lowers `%p.b` and `%p.c` as byval
  aggregate pointers in `fa3`, clears the later idea-65 direct-call and
  idea-66 load-local-memory leaves, and now fails in function `myprintf` with
  `scalar-control-flow semantic family`, so its current owned route is back in
  idea 58's broader semantic-lowering lane unless a narrower scalar-control-flow
  leaf is opened.
- `c_testsuite_x86_backend_src_00040_c` now fails in `chk` in the `gep
  local-memory semantic family` and is better described by idea 62's
  stack/addressing ownership.
- `c_testsuite_x86_backend_src_00045_c` now reaches x86 prepared emission and
  fails under idea 60's minimal single-block terminator restriction.
- `c_testsuite_x86_backend_src_00088_c` now reaches x86 prepared emission and
  fails under idea 59's authoritative prepared guard-chain handoff family.

## Scope Notes

This leaf is about upstream semantic coverage, not about adding more x86
matcher-shaped escapes.

Expected repair themes include:

- semantic lowering for currently unadmitted frontend/backend route families
- BIR shaping that reaches prepared-x86 handoff instead of aborting early
- generalization across nearby failing cases, not one named testcase at a time

## Boundaries

This idea does not own:

- prepared short-circuit or guard-chain consumption once semantic lowering is
  already available
- call-bundle consumption inside x86 prepared emission
- multi-function prepared-module consumption
- runtime correctness bugs after codegen succeeds

Dynamic local/member/array addressing cases may start here, but when the durable
problem is better described as stack/addressing semantics they should be owned
by idea 62 instead of staying mixed into this broader bucket.

## Dependencies

This is one of the earliest leaves in the series. Downstream x86 emitter work
should not claim completion over cases still blocked by this upstream family.

## Completion Signal

This idea is complete when the owned semantic-lowering family is either:

- repaired so those cases reach prepared-x86 consumption, or
- durably rehomed into a more precise open idea with the source notes updated

Boundary tests alone are insufficient proof.
