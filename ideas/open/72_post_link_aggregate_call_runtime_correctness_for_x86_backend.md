# Post-Link Aggregate Call Runtime Correctness For X86 Backend

Status: Open
Created: 2026-04-22
Last-Updated: 2026-04-22
Parent Idea: [71_post_link_variadic_runtime_correctness_for_x86_backend.md](/workspaces/c4c/ideas/open/71_post_link_variadic_runtime_correctness_for_x86_backend.md)

## Intent

Repair the next post-link x86 backend runtime leaf revealed by `00204.c`: the
program now links, but the first execution crash is in fixed-arity
aggregate-by-value call behavior before the variadic path begins.

## Owned Failure Family

This idea owns x86 backend failures where:

- the prepared-module route already matches
- final assembly is already valid
- post-assembly closure already resolved the relevant same-module and direct
  variadic-runtime references well enough for the program to link
- the next blocker is runtime correctness for fixed-arity aggregate argument or
  result handling after link, such as by-value aggregate home selection, call
  setup, or fixed-arity helper forwarding before later variadic traversal

## Current Known Failed Cases It Owns

- `c_testsuite_x86_backend_src_00204_c`

## Latest Durable Note

As of 2026-04-22, the fresh focused proof

`ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$'`

shows `backend_x86_handoff_boundary` passing while `00204.c` still fails at
runtime with `Segmentation fault`. A `gdb` backtrace against
`build/c_testsuite_x86_backend/src/00204.c.bin` identifies the first crash in
`fa_s2`, reached from `arg`, before the program enters the later variadic
helper path. The emitted code in that helper dereferences a bogus pointer from
`(%rsp)` and forwards `%r12` to `printf`, which points to a fixed-arity
aggregate call/runtime ABI seam rather than a `va_start` or `va_list` defect.

## Scope Notes

Expected repair themes include:

- truthful runtime semantics for fixed-arity aggregate-by-value argument and
  return handling after codegen already succeeds
- correct aggregate home selection, call setup, and forwarding for helper
  functions such as `fa_s*`
- runtime-only proof that distinguishes aggregate call ABI correctness from
  earlier prepared-module or post-assembly closure work

## Boundaries

This idea does not own:

- prepared-module rejection before assembly; that remains in idea 61
- unresolved same-module or direct variadic-runtime references before link;
  those were idea 70's closure seam
- genuine post-link variadic runtime failures in `myprintf`, `llvm.va_start`,
  or `va_list` traversal; those remain in idea 71
- generic non-call runtime correctness work unrelated to the graduated
  aggregate-call route

## Completion Signal

This idea is complete when the owned post-link aggregate-call runtime cases no
longer crash at their current fixed-arity aggregate ABI seam and instead
either execute correctly or graduate into a later, better-fitting runtime leaf.
