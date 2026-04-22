# Post-Link Byval-Param Pointer-Argument Runtime Correctness For X86 Backend

Status: Closed
Created: 2026-04-22
Last-Updated: 2026-04-22
Closed: 2026-04-22
Disposition: Completed by exhausting the owned byval-param pointer-argument runtime family and rehoming `00204.c` into a later prepared/x86 call-lane clobber leaf.
Parent Idea: [73_post_link_address_exposed_local_home_runtime_correctness_for_x86_backend.md](/workspaces/c4c/ideas/closed/73_post_link_address_exposed_local_home_runtime_correctness_for_x86_backend.md)

## Intent

Repair the next post-link x86 backend runtime leaf revealed by `00204.c`: the
`%lv.s` local-home mismatch in `myprintf` is cleared, but fixed-arity helpers
such as `fa_s1` still materialize pointer arguments from the wrong home for
address-exposed `byval_param` values.

## Owned Failure Family

This idea owns x86 backend failures where:

- the prepared-module route already matches
- final assembly is already valid and the program links
- earlier post-link aggregate-call transport and address-exposed `local_slot`
  permanent-home seams already cleared
- the next blocker is runtime correctness for address-exposed
  `byval_param` values with permanent home slots whose emitted pointer
  arguments are materialized from a transient register or stale lane instead of
  the authoritative byval home

## Current Known Failed Cases It Owns

- `c_testsuite_x86_backend_src_00204_c`

## Latest Durable Note

As of 2026-04-22, fresh focused proof

`ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$'`

shows `backend_x86_handoff_boundary` passing while `00204.c` still fails at
runtime with `[RUNTIME_MISMATCH]` in the fixed-arity `Arguments:` phase, not
at the old `myprintf` / `match(&s, ...)` crash. Generated
`build/c_testsuite_x86_backend/src/00204.c.s` now keeps `%lv.s` on `[rsp]`,
but helpers such as `fa_s1` store incoming byval pointer `%p.a` at `[rsp]`
while later calling `printf("%.1s\n", a.x)` with `mov rsi, r12` instead of the
address of `a.x`. Focused prepared evidence confirms `%p.a` is
`source_kind=byval_param`, `address_exposed=yes`, `permanent_home_slot=yes`,
with fixed stack `slot #0 offset=0`, while the emitted pointer operand `%t2`
is homed to register `r12` and forwarded through the ABI move bundle. Durable
ownership therefore graduates out of idea 73 and into this narrower post-link
runtime byval-param pointer-materialization leaf.

## Scope Notes

Expected repair themes include:

- truthful runtime semantics for authoritative home selection when
  address-exposed `byval_param` values need by-reference pointer arguments
- correct agreement between fixed byval home metadata and emitted pointer
  arguments sent to external or helper calls from fixed-arity paths such as
  `fa_s*`
- focused runtime proof that distinguishes byval-param pointer materialization
  from earlier aggregate-call transport, local-slot permanent-home placement,
  and later variadic traversal work

## Boundaries

This idea does not own:

- aggregate-by-value argument or result transport bugs already exhausted under
  idea 72
- address-exposed `local_slot` permanent-home disagreement such as `%lv.s` in
  `myprintf`; that was idea 73
- pre-codegen prepared local-slot handoff rejection; that remains in idea 68
- genuine post-link variadic traversal defects in `llvm.va_start`, `va_list`,
  or later helper walking; those return to idea 71

## Completion Signal

This idea is complete when the owned post-link byval-param pointer-argument
runtime cases no longer fail at their current fixed-arity helper materialized
pointer/home mismatch and instead either execute correctly or graduate into a
later, better-fitting runtime leaf.

## Closure Note

Closed on 2026-04-22 after the owned byval-param pointer-argument runtime
family was exhausted for the only confirmed case:

- `c_testsuite_x86_backend_src_00204_c` no longer stops at the earlier
  fixed-arity helper pointer/home mismatch; semantic BIR now publishes
  `printf(..., ptr %lv.param.a.0)` for `fa_s1` through `fa_s16` and
  `printf(..., ptr %p.a)` for `fa_s17`
- focused proof still shows `backend_x86_handoff_boundary` passing while
  `00204.c` fails later with `[RUNTIME_MISMATCH]`
- the first remaining bad fact now lives in the prepared/x86 call-lane
  consumer: generated asm for `fa_s17` emits `mov rsi, rdi` after loading the
  format string into `rdi`, so arg1 is copied from the clobbered format
  register instead of the authoritative `%p.a` owner
- the same later overlapping-lane clobber also appears in `pll`, which shows
  the remaining blocker is a downstream prepared/x86 call-lane runtime leaf,
  not an idea-74 byval-param owner-publication defect

## Validation At Closure

Close-time guard used the existing focused runtime scope:

- `cmake --build --preset default`
- `ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$' | tee test_after.log`
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result:

- guard passed for lifecycle-only closure with equal pass count allowed
- before reported `1` passed / `1` failed / `2` total
- after reported `1` passed / `1` failed / `2` total
- no new failing tests were introduced on the matched scope
