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

- `c_testsuite_x86_backend_src_00204_c`

## Latest Durable Note

As of 2026-04-22, idea 70's closure repair in
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` removed the unresolved
same-module `s1`..`s16` references and the unresolved `llvm.va_start.p0`
failure for `00204.c`. Fresh focused proof now links that case successfully
and reaches a later runtime failure:

- `c_testsuite_x86_backend_src_00204_c` exits via `[RUNTIME_NONZERO]`
- the executable crashes with `Segmentation fault`

That runtime crash is a separate initiative from idea 70's remaining
post-assembly boundary-contract work and should not be silently absorbed back
into the closure leaf.

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
