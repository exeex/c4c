# AArch64 Backend Non-Leaf Call-Frame LR Preservation Todo

Status: Active
Source Idea Path: ideas/open/285_aarch64_backend_nonleaf_call_frame_lr_preservation.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Reclassify Timeout Boundary

# Current Packet

## Just Finished

Completed plan.md Step 4, "Prove Simple Runtime Probes", using the targeted
runtime probe evidence after the Step 3 LR preservation repair. The old
link-register timeout owner is gone from the clean probe set:

- `00100.c` passed.
- `00121.c` passed.
- `00116.c` no longer times out and no longer has the LR-clobber shape, but
  fails with `RUNTIME_NONZERO exit=1`; treat that as a different semantic owner
  for follow-on work, not as Step 4 LR-proof failure.

Regenerated runtime probe assembly now shows `00121.c` `main` with a real LR
frame around both calls:
`sub sp, sp, #32`, `str x30, [sp, #16]`, `bl f`, `bl g`,
`ldr x30, [sp, #16]`, `add sp, sp, #32`, `ret`. The old LR-clobber timeout
shape is gone.

## Suggested Next

Execute plan.md Step 5 by reclassifying the original 23-case timeout bucket
after LR preservation. Separate cases fixed by LR preservation from remaining
timeouts or non-timeout failures, and record follow-on semantic owners such as
the `00116.c` `RUNTIME_NONZERO exit=1` argument/value behavior outside this LR
route instead of implementing that behavior here.

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
