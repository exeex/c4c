# Execution State

Status: Active
Source Idea Path: ideas/open/70_post_asm_global_function_pointer_and_variadic_runtime_link_closure_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Repair The Selected Post-Asm Closure Seam
Plan Review Counter: 0 / 4
# Current Packet

## Just Finished

Step `2.1` repaired the first post-assembly link-closure gap in
`src/backend/mir/x86/codegen/prepared_module_emit.cpp`: the emitted x86 module
now closes over the missing same-module symbol definitions that left
`c_testsuite_x86_backend_src_00204_c` with unresolved `s1`..`s16`, and it
also emits direct variadic-runtime helper symbols such as `llvm.va_start.p0`
so the case advances beyond the prior unresolved-reference family. With that
closure seam repaired, `00204.c` now links and reaches a later downstream
runtime crash inside the variadic path instead of failing at link.

## Suggested Next

Start the next Step `2.1` packet on truthful variadic-runtime semantics for
the emitted `llvm.va_start.p0` closure path so the newly linked `00204.c`
advances beyond its current runtime segfault, while separately rechecking the
nearest boundary expectation against the actual rejection wording before any
test update.

## Watchouts

- Keep the `fld` / `fstp` long-double asm-emission repair in place; idea 69 is
  closed and should not be reopened unless assembler-invalid output returns.
- The link-time unresolved-reference family for `00204.c` is cleared; the next
  observed blocker is a runtime segfault after the new helper closure lets the
  binary link.
- The variadic helper closure is only a first ownership repair. `llvm.va_start`
  now resolves, but the helper semantics are not yet sufficient to preserve
  correct runtime behavior for the full `myprintf` path.
- `backend_x86_handoff_boundary` still fails on contract wording, not on the
  repaired same-module symbol or direct variadic-runtime unresolved-reference
  seam.

## Proof

Delegated executor proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$' > test_after.log || true`
Result: build stayed green, `test_after.log` no longer reports unresolved
`s1`..`s16` or unresolved `llvm.va_start.p0` for `00204.c`, and that case now
fails later with `[RUNTIME_NONZERO]` / `Segmentation fault`. The focused
boundary test still fails on contract wording.
