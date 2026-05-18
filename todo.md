# AArch64 Backend Non-Leaf Call-Frame LR Preservation Todo

Status: Active
Source Idea Path: ideas/open/285_aarch64_backend_nonleaf_call_frame_lr_preservation.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Simple Runtime Probes

# Current Packet

## Just Finished

Completed the follow-up for plan.md Step 3, "Repair LR Preservation For
Non-Leaf Functions", covering and repairing the combined non-leaf plus
saved-register frame shape that still left `00121.c` with `bl f`, `bl g`, and
a bare `ret`.

Extended the focused backend coverage so target-instruction, machine-printer,
and return-lowering tests assert LR preservation still works when a frame also
carries a prepared saved `x19` slot. Extended the repair so AArch64 frame
boundary insertion no longer suppresses LR frames just because prepared
saved-callee registers exist, computes the combined frame extent from prepared
saved-register slots plus the LR save slot, and prints saved-register
`str`/`ldr` lines when complete slot facts are present.

Regenerated runtime probe assembly now shows `00121.c` `main` with a real LR
frame around both calls:
`sub sp, sp, #32`, `str x30, [sp, #16]`, `bl f`, `bl g`,
`ldr x30, [sp, #16]`, `add sp, sp, #32`, `ret`. The old LR-clobber timeout
shape is gone.

## Suggested Next

Execute plan.md Step 4 follow-up focused on the remaining non-LR owner:
`00116.c` now has an LR frame and exits nonzero (`RUNTIME_NONZERO exit=1`).
Investigate argument/value return behavior for `f(0)` without changing
c-testsuite contracts or expanding back into timeout handling.

## Watchouts

- Do not special-case c-testsuite filenames, function names, exact emitted text,
  timeout settings, allowlists, unsupported classifications, or expected
  outputs.
- Keep printf, string literals, variadic calls, loop predicates,
  short-circuiting, aggregates/pointers, static globals, and goto behavior out
  of this LR preservation route.
- Use explicit timeouts for runtime probes and check for stale generated
  runtime processes after timeout-oriented runs.
- Dynamic-stack frames remain outside this printable LR subset and still fail
  closed instead of guessing a combined dynamic frame layout.
- `00121.c` passes the targeted runtime probe after this repair. Its
  regenerated assembly uses `x19` as the live value across calls, while the
  prepared frame facts visible to this packet only required the combined frame
  extent and LR save slot in the generated output.
- `00116.c` is no longer an LR timeout shape: regenerated `main` saves/restores
  `x30` around `bl f`, then returns. The remaining exit=1 should be treated as
  an argument/value return semantic owner.

## Proof

Ran the delegated proof command exactly, with combined output in
`test_after.log`:

```sh
cmake --build build --target backend_aarch64_target_instruction_records_test backend_aarch64_machine_printer_test backend_aarch64_return_lowering_test > test_after.log 2>&1 && ctest --test-dir build -R 'backend_aarch64_(target_instruction_records|machine_printer|return_lowering)$' --output-on-failure >> test_after.log 2>&1
```

Result: passed. `ctest` ran
`backend_aarch64_target_instruction_records`,
`backend_aarch64_machine_printer`, and `backend_aarch64_return_lowering` with
0 failures.

Then ran the delegated runtime probe command separately, not written to
`test_after.log`:

```sh
cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan -R 'c_testsuite_aarch64_backend_src_00(100|116|121)_c$' --output-on-failure --timeout 10
```

Result: `00100.c` passed, `00121.c` passed, and `00116.c` failed with
`RUNTIME_NONZERO exit=1`. The runtime command therefore exited nonzero because
of the remaining `00116.c` semantic owner, not because of the old LR timeout
shape.
