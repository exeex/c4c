Status: Active
Source Idea Path: ideas/open/324_aarch64_variadic_frame_formal_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Frame And Formal Publication

# Current Packet

## Just Finished

Step 2 repaired the AArch64 variadic frame/formal publication faults localized
in Step 1 and then refined the repair after reviewer feedback.

Implemented:

- Regalloc stack-slot allocation now starts from the current function frame
  extent instead of the module-global frame size.
- Regalloc publishes assigned stack homes as function-local frame slots, so
  later variadic helper frame slots cannot overlap them and frame planning can
  size the active function from the same homes that call preservation consumes.
- `populate_frame_plan()` now includes assigned regalloc stack homes in
  `PreparedFramePlanFunction::frame_size_bytes` and frame-slot order.
- Variadic fixed-formal value homes report their assigned live home when
  regalloc rehomes them, instead of continuing to report stale ABI entry
  registers.
- AArch64 dispatch publishes fixed-formal stack homes at variadic function
  entry before inline variadic helper lowering.
- Entry fixed-formal publication now uses typed store widths:
  `strb` for `I1`/`I8`, `strh` for `I16`, `str` with `W`/`X` register views
  for `I32`/`I64`/`Ptr`, and `str` with `S`/`D` register views for
  `F32`/`F64`.
- AArch64 dispatch suppresses normal before-call ABI moves for inline variadic
  entry helpers, so `llvm.va_start.p0` no longer clobbers fixed formals through
  ordinary call-argument movement before helper lowering.
- Focused backend coverage now proves fixed-formal publication order and typed
  store width before inline `va_start` lowering, plus frame-plan coverage for
  regalloc stack homes consumed by call preservation.

Fresh generated evidence in `build/c_testsuite_aarch64_backend/src/00204.c.s`:

- `myprintf` now allocates `1408` bytes and its prior out-of-frame access
  family such as `[sp, #9696]` is absent from `myprintf`.
- `%p.format` is published with `str x0, [sp, #624]` before `va_start`.
- `myprintf` no longer emits the bad entry `mov x0, x21`.
- The variadic register-save area now starts at `stack+1136`, so it no longer
  overlaps `%p.format` at `stack+624`.
- Raw `va.arg.aggregate*` helper text remains absent.

The focused external representative still fails with
`[RUNTIME_NONZERO] exit=Segmentation fault`, now with empty captured
stdout/stderr in this proof run. The remaining fault is no longer the localized
frame/formal publication issue: generated stack references observed in the
current artifacts are covered by their emitted frame sizes, and the entry
fixed-formal clobber is gone.

## Suggested Next

Run Step 3 or lifecycle classification at supervisor discretion. The next
packet should classify the remaining external runtime segfault as the next
owner after verifying the current generated failure is not another
frame/formal publication defect.

## Watchouts

- Preserve prior repairs: large stack offsets, large frame adjustments,
  `va_start` helper lowering, scalar ALU immediates, HFA argument lanes, F128
  transport, aggregate helper text lowering, `va_start` destination
  materialization, and aggregate/floating `va_arg` source/progression.
- Raw `va.arg.aggregate*` text must stay absent.
- HFA/floating aggregate consumers should keep FP register-save-area source
  selection with `FpOffset` progression and overflow fallback unless generated
  evidence proves that path still owns the first bad fact.
- Do not special-case `00204.c`, `stdarg`, `myprintf`, `format`, `x0`, `x21`,
  one local, one stack slot, one offset, or one emitted instruction sequence.
- Do not weaken expectations, unsupported classifications, CTest registration,
  runner behavior, timeout policy, proof-log policy, or prior guardrails.
- Raw `va.arg.aggregate*` text is still absent.
- The current generated `stdarg` function contains large temporary HFA/vector
  stack traffic, but its prologue uses a large register-mediated frame
  adjustment that covers the observed offsets. Do not treat those large offsets
  as the old frame-size bug without checking the function-local prologue.
- The smallest representative local surfaces are prepared-BIR/function summary
  dumps for `00204.c`, `backend_aarch64_machine_printer`,
  `backend_aarch64_target_instruction_records`, and
  `backend_aarch64_instruction_dispatch`.

## Proof

Ran the delegated Step 2 focused proof into `test_after.log`:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)|backend_cli_dump_(bir|prepared_bir)_(00204_stdarg_(semantic|prepared)_handoff|focus_(function_filters|block_entry)_00204)|c_testsuite_aarch64_backend_src_00204_c)$'`

Build succeeded. CTest ran 11 tests: 10 passed, 1 failed. Internal focused
backend tests passed. The only failure is
`c_testsuite_aarch64_backend_src_00204_c`, still
`[RUNTIME_NONZERO] exit=Segmentation fault`.

Proof log path: `test_after.log`.

Supplementary owned coverage run:

`ctest --test-dir build -j --output-on-failure -R '^backend_prepare_frame_stack_call_contract$'`

Passed: 1/1.
