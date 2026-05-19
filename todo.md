Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Classified HFA/Floating Owner

# Current Packet

## Just Finished

Step 2 repaired the owned AArch64 global data emission path in
`src/backend/mir/aarch64/codegen/asm_emitter.cpp`: `F32` immediate
initializers now emit `.word` IEEE bit-pattern data and `F64` immediate
initializers now emit `.xword` IEEE bit-pattern data. The same directive path
is used for scalar globals and aggregate initializer elements, so HFA-shaped
globals are covered generally rather than by named-case matching.

Focused backend coverage in `backend_aarch64_instruction_dispatch` now builds
a prepared AArch64 module with scalar `F32`/`F64` globals and aggregate
`F32`/`F64` initializer elements, then verifies the printed assembly contains
real data and no deferred global-data marker.

Fresh `00204.c` generated assembly now emits real data for `hfa11` through
`hfa24`; for example `hfa11` emits `.word 1093769626`, `hfa12` emits two
`.word` values, and `hfa21` through `hfa24` emit `.xword` values. The
representative still fails at runtime. The new first bad fact is no longer
global initializer data: the direct `fa_hfa11(hfa11)` producer loads `hfa11`
into `s13` but reaches `bl fa_hfa11` without moving/publishing that lane into
the ABI argument register `s0`, while `fa_hfa11` consumes `s0`.

## Suggested Next

Next packet should repair direct fixed HFA floating call-lane publication for
AArch64: when a fixed HFA argument lane is loaded or materialized in a
non-ABI FPR such as `s13`, the call-boundary lowering must move it into the
classified ABI argument register such as `s0` before the call.

## Watchouts

- Do not reopen global initializer lowering unless fresh evidence shows missing
  BIR initializer data. Current generated assembly proves the AArch64 emitter
  now materializes `F32`/`F64` HFA global data.
- The next observed bad call path is not variadic: `arg` loads `hfa11` and
  directly calls `fa_hfa11`, so fixed-argument HFA lane publication should be
  checked before register-save-area, overflow-area, or `va_arg` progression.
- Preserve prior repairs: large stack offsets, large frame adjustments,
  `va_start` helper lowering, scalar ALU immediates, HFA argument lanes, F128
  transport, aggregate helper text lowering, `va_start` destination
  materialization, aggregate/floating `va_arg` source/progression, frame-size
  coverage, fixed-formal publication, and local/value-home publication.
- Branch publication for the `for.cond.70` fused compare, literal pointer
  provenance for `%t5`/`@.str30`, frame-slot address call operands for
  `%lv.t7.0`, stale mutable pointer-local provenance, later branch exits, and
  `%t15` predecessor/join source publication are fixed and now have focused
  local coverage.
- The remaining runtime representative failure is still
  `RUNTIME_NONZERO`/segmentation fault after printing substantial output.
  After the global-data repair, the first HFA float argument still prints
  `0.0` instead of `11.1` because the producer never moves the loaded lane into
  `s0` before `bl fa_hfa11`.
- Do not special-case `00204.c`, `stdarg`, `myprintf`, the format loop, one
  HFA shape, one float literal, one stack slot, one register, one offset, or
  one emitted instruction sequence.
- Do not weaken expectations, unsupported classifications, CTest registration,
  runner behavior, timeout policy, proof-log policy, or prior guardrails.
- Reopen frame/formal publication only if fresh generated output again shows
  uncovered stack references or fixed-formal clobber before publication.
- Reopen local/value-home publication only if fresh generated output again
  shows an unpublished ordinary local, constant, pattern operand, branch
  condition, call operand, or predecessor/join source.

## Proof

Delegated proof run:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records|c_testsuite_aarch64_backend_src_00204_c)$'`

Result: build succeeded; 3/4 focused CTest tests passed. The focused backend
tests `backend_aarch64_machine_printer`,
`backend_aarch64_instruction_dispatch`, and
`backend_aarch64_target_instruction_records` passed. The representative
`c_testsuite_aarch64_backend_src_00204_c` still fails with
`RUNTIME_NONZERO`/segmentation fault after the first bad fact moved from
missing global data to missing fixed-HFA call-lane publication into `s0`.

Proof log: `test_after.log`.
