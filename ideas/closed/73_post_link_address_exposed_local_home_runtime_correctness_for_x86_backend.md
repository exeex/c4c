# Post-Link Address-Exposed Local-Home Runtime Correctness For X86 Backend

Status: Closed
Created: 2026-04-22
Last-Updated: 2026-04-22
Closed: 2026-04-22
Disposition: Completed by exhausting the owned address-exposed local-home runtime family and rehoming `00204.c` into a narrower downstream byval-param pointer-materialization leaf.
Parent Idea: [72_post_link_aggregate_call_runtime_correctness_for_x86_backend.md](/workspaces/c4c/ideas/closed/72_post_link_aggregate_call_runtime_correctness_for_x86_backend.md)

## Intent

Repair the next post-link x86 backend runtime leaf revealed by `00204.c`: the
program now advances past the aggregate-call handoff, but the first bad fact is
inside same-module helper `myprintf`, where an address-exposed local with a
permanent home slot is stored and later referenced through disagreeing stack
homes.

## Owned Failure Family

This idea owns x86 backend failures where:

- the prepared-module route already matches
- final assembly is already valid and the program links
- earlier post-link aggregate-call/runtime handoff seams already cleared
- the next blocker is runtime correctness for address-exposed locals that
  require authoritative permanent home slots in emitted same-module helpers,
  such as by-reference helper calls observing a different stack home than the
  live local value

## Current Known Failed Cases It Owns

- `c_testsuite_x86_backend_src_00204_c`

## Latest Durable Note

As of 2026-04-22, fresh focused proof

`ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$'`

still shows `backend_x86_handoff_boundary` passing while `00204.c` fails at
runtime with `Segmentation fault`. The first bad stop is now `match()` called
from `myprintf`: `%rsi` carries a valid pattern pointer, but the caller-supplied
`*s` is already corrupt, with `match` faulting at `movsbl (%rax), %eax` after
loading `%rax == 0x000000b000000030`. Generated `myprintf` asm stores the live
format cursor with `mov QWORD PTR [rsp], rax` but still passes `&s` to
`match(&s, ...)` as `lea rdi, [rsp + 80]`. Prepared metadata for `%lv.s` says
`address_exposed=yes`, `requires_home_slot=yes`, and
`permanent_home_slot=yes`, so durable ownership has graduated out of idea 72's
aggregate-call family and into this narrower post-link runtime local-home leaf.

## Scope Notes

Expected repair themes include:

- truthful runtime semantics for authoritative permanent-home selection and
  refresh of address-exposed locals in emitted same-module helpers
- correct agreement between the live local value location and the by-reference
  address passed to helper calls such as `match(&s, ...)`
- focused runtime proof that distinguishes local-home placement bugs from
  earlier aggregate-call seams and later `va_start` / `va_list` traversal work

## Boundaries

This idea does not own:

- aggregate-by-value argument or result handoff bugs already exhausted under
  idea 72
- pre-codegen prepared local-slot handoff rejection; that remains in idea 68
- generic call/prologue ownership unless fresh proof moves the first bad fact
  back into those shared paths
- genuine post-link variadic traversal defects in `llvm.va_start`, `va_list`,
  or later variadic walking; those return to idea 71

## Completion Signal

This idea is complete when the owned post-link address-exposed local-home
runtime cases no longer crash at their current same-module helper home mismatch
and instead either execute correctly or graduate into a later, better-fitting
runtime leaf.

## Closure Note

Closed on 2026-04-22 after the owned address-exposed local-home runtime family
was exhausted for the only confirmed case:

- `c_testsuite_x86_backend_src_00204_c` no longer stops at the prior
  `myprintf` `%lv.s` permanent-home disagreement; generated asm now stores the
  live cursor at `[rsp]` and calls `match(&s, ...)` with `lea rdi, [rsp]`
- focused proof still shows `backend_x86_handoff_boundary` passing while
  `00204.c` fails later as `[RUNTIME_MISMATCH]` in the fixed-arity
  `Arguments:` phase
- the first remaining bad fact now lives in helpers such as `fa_s1`, where
  prepared metadata records `%p.a` as an address-exposed `byval_param` with a
  fixed permanent home at stack offset `0`, but emitted pointer arguments still
  forward transient register `%t2` / `%r12` into `printf`
- that is a narrower post-link runtime byval-param pointer-argument
  materialization seam, not an idea-73 `local_slot` permanent-home defect

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
