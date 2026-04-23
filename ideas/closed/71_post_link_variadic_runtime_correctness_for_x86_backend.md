# Post-Link Variadic Runtime Correctness For X86 Backend

Status: Open
Created: 2026-04-22
Last-Updated: 2026-04-22
Parent Idea: [70_post_asm_global_function_pointer_and_variadic_runtime_link_closure_for_x86_backend.md](/workspaces/c4c/ideas/open/70_post_asm_global_function_pointer_and_variadic_runtime_link_closure_for_x86_backend.md)

## Intent

Repair the next x86 backend leaf that appears after idea 70's post-assembly
closure work succeeds: `00204.c` now links, but the emitted program still
crashes at runtime in the variadic path.

## Owned Failure Family

This idea owns x86 backend failures where:

- the prepared-module route already matches
- final assembly is already valid
- post-assembly closure already resolved the relevant same-module symbol and
  direct variadic-runtime references well enough for the program to link
- the next blocker is runtime correctness for the emitted variadic path, such
  as wrong `llvm.va_start.p0` semantics, wrong argument walking, or later
  runtime crashes in `myprintf`

## Current Known Failed Cases It Owns

- none currently confirmed; `c_testsuite_x86_backend_src_00204_c` graduated on
  2026-04-22 into a separate fixed-arity aggregate-call runtime leaf after the
  first post-link crash proved not to be variadic-path-owned

## Latest Durable Note

As of 2026-04-22, idea 70's closure repair in
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` still stands: fresh
focused proof

`ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$'`

shows `backend_x86_handoff_boundary` passing while `00204.c` links and then
fails later at runtime. A follow-on `gdb` backtrace against the generated
`build/c_testsuite_x86_backend/src/00204.c.bin` shows the first crash in
fixed-arity helper `fa_s2`, not in `myprintf`, `llvm.va_start.p0`, or later
`va_list` traversal. The emitted code dereferences a bogus pointer from
`(%rsp)` and forwards `%r12` to `printf`, so `00204.c` no longer belongs to
this variadic-runtime leaf.

Durable ownership for that case therefore graduates into a separate post-link
fixed-arity aggregate-call runtime leaf. Keep idea 71 focused on genuine
variadic runtime failures after link succeeds, and only rehome `00204.c` back
here if its first runtime blocker advances into `va_start`, `va_list`, or
later `myprintf`-owned execution.

## Scope Notes

Expected repair themes include:

- truthful runtime semantics for emitted variadic helper closures
- correct `va_list` setup and traversal after the program already links
- runtime-only proof that distinguishes link closure from execution
  correctness

## Boundaries

This idea does not own:

- prepared-module rejection before assembly; that remains in idea 61
- stale x86 handoff contract wording for the post-assembly closure boundary;
  that remains in idea 70 / idea 67-adjacent coverage
- unresolved same-module or direct variadic-runtime references before link;
  those were the idea-70 closure seam
- generic non-variadic runtime correctness work unrelated to the graduated
  `00204.c` path

## Completion Signal

This idea is complete when the owned post-link variadic runtime cases no
longer crash at runtime and instead either run successfully or graduate into a
later, better-fitting runtime correctness leaf.
