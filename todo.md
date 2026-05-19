Status: Active
Source Idea Path: ideas/open/299_aarch64_scalar_immediate_materialize_or_encoding_fallback.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove focused and residual behavior

# Current Packet

## Just Finished

Step 3 proved the scalar ALU immediate materialization slice after a fresh
build. The focused subset remains red at 1/8 passing, but it no longer reports
the original scalar immediate printer diagnostic
`scalar add/sub/bitwise immediate operand is outside the plain #imm encoding
range 0..4095` for any focused case. Focused movement is unchanged from the
implementation slice: `00031` passes; `00104` fails on invalid cast spelling
`sxtw w20, w13`; `00213`/`00214` fail on symbol-store value printing;
`00207`/`00215` segfault; `00143` times out; and `00218` reaches a runtime
mismatch (`unsigned enum bit-fields broken`).

The broader backend-regex inventory ran after that focused movement and
returned 294/352 passing, 58 failing. Residual buckets in `test_after.log` are:
21 `FRONTEND_FAIL`, 1 `BACKEND_FAIL`, 20 `RUNTIME_NONZERO`, 14
`RUNTIME_MISMATCH`, and 2 timeouts. The old scalar immediate printer diagnostic
is absent from the broad log as well. The remaining broad frontend failures are
outside this owner: scalar cast printer gaps, scalar mul/div printer gaps,
memory store/source printing, call-boundary move, unsigned reduction shift, and
semantic `lir_to_bir` gaps.

No stale qemu, c-testsuite, or c4c runtime process related to this test run was
present before or after the broad runtime command, so no process was
terminated.

## Suggested Next

Idea 299 can close if the supervisor accepts that the scalar ALU immediate
printer objective is proven: the original diagnostic is gone from focused and
broad validation, and all remaining focused failures are classified outside the
scalar immediate materialization owner. If not closing immediately, the blocker
is not in idea 299; the next coherent owner should target one residual bucket,
most likely symbol-store value printing or the invalid `sxtw w20, w13` cast
spelling.

## Watchouts

- The delegated proof is still red because the broader backend-regex inventory
  includes known residual buckets, not because the scalar immediate diagnostic
  remains.
- `test_after.log` now contains the broad backend-regex proof, not just the
  focused subset.
- Do not treat remaining runtime, timeout, cast, memory-store, mul/div,
  call-boundary, or semantic-lowering failures as scalar immediate
  materialization failures without fresh generated-code evidence.

## Proof

Ran the delegated proof sequence exactly:

`cmake --build --preset default`

`ctest --test-dir build -j8 -R '^c_testsuite_aarch64_backend_src_(00031|00104|00143|00207|00213|00214|00215|00218)_c$' --output-on-failure`

`timeout 900s ctest --test-dir build -j10 -R backend --output-on-failure > test_after.log 2>&1`

Build completed with no work to do. Focused CTest returned failure status with
1/8 passing and 7/8 failing, all outside the original scalar immediate printer
diagnostic. Broad backend-regex CTest returned failure status with 294/352
passing and 58/352 failing. The canonical proof log is `test_after.log`.
