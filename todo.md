Status: Active
Source Idea Path: ideas/open/336_aarch64_return_result_publication_epilogue_clobber.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Narrow Return Publication Owner

# Current Packet

## Just Finished

Step 2 repaired the AArch64 return publication owner in dispatch. A
`before_return` FunctionReturnAbi move is now skipped when the same value has
already been emitted into the ABI return register, and retained
`before_return` publications update scalar state before terminal return
lowering so the final `ReturnInstructionRecord` no longer reloads a stale home
after frame teardown. Return printing also treats same physical return
register aliases as already in the ABI register instead of printing a
redundant terminal self-move.

Focused coverage was added in `backend_aarch64_return_lowering_test` for:

- direct load return publication where the stale prepared-home
  `before_return` copy must be suppressed
- call-result return publication where `mov x0, x21` before epilogue is the
  authoritative move and the terminal return must not emit `mov w0, w21` after
  `x21` is restored

Representative generated results after the repair:

- `00004`, `00011`, and `00087` pass the delegated c-testsuite subset.
- `00004` and `00011` no longer show the correct `w0` result followed by stale
  `mov x0, x20` / `mov x0, x13` home publication.
- `00087` keeps the intended `mov x0, x21` before the epilogue and no longer
  emits the stale post-epilogue `mov w0, w21`.
- `00168` advances past the stale return-publication overwrite: recursive
  `factorial` now computes `mul w0, w19, w21` and returns directly from `w0`.
  Its remaining runtime mismatch is a separate live callee-saved home problem:
  `w19` holds the caller's `n` across `bl factorial` but the generated frame
  does not save/restore `x19`, so recursive returns leave `w19 == 1` and every
  printed factorial result is `1`.

## Suggested Next

Smallest next packet: classify or repair the `00168` residual as live
callee-saved scalar-home preservation across recursive calls, starting from
the frame/register-preservation facts that omit `x19` while the recursive
factorial body keeps `n` in `w19` across `bl factorial`.

## Watchouts

Do not treat `00087`'s remaining `mov x0, x21` before the restore sequence as
stale; that is the intended `before_return` publication. The stale pattern was
the later terminal home reload after `x21` had been restored. The remaining
`00168` failure is not a return-publication overwrite and should not be fixed
with return-move suppression or testcase-specific register strings.

## Proof

`cmake --build build --target c4cll backend_aarch64_return_lowering_test backend_aarch64_scalar_alu_records_test backend_aarch64_instruction_dispatch_test backend_aarch64_machine_printer_test -j 2 && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_(return_lowering|scalar_alu_records|instruction_dispatch|machine_printer)|c_testsuite_aarch64_backend_src_(00004|00011|00087|00168)_c' > test_after.log 2>&1`

Result: build succeeded; backend focused tests passed; c-testsuite
representatives `00004`, `00011`, and `00087` passed; `00168` failed with the
residual callee-saved `w19` preservation issue classified above. Canonical
proof log: `test_after.log`.

Supervisor follow-up validation accepted the advanced residual: the focused
return-publication subset
`backend_aarch64_(return_lowering|scalar_alu_records|instruction_dispatch|machine_printer)|c_testsuite_aarch64_backend_src_(00004|00011|00022|00052|00087)_c`
passed 9/9, and broader internal backend validation
`ctest --test-dir build -j --output-on-failure -R '^backend_'` passed 141/141.
