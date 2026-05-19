Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Classified HFA/Floating Owner

# Current Packet

## Just Finished

Step 2 repaired AArch64 fixed HFA/floating call-lane publication in
`src/backend/mir/aarch64/codegen/calls.cpp`: scalar FPR call-boundary argument
moves are now selected when prepared call ownership says a lane in a non-ABI
FPR must be published into the classified ABI FPR, and the printer emits
`fmov` for scalar `s`/`d` FPR register-to-register call-boundary moves.

Focused backend coverage now proves the record and printer contracts:
`backend_aarch64_target_instruction_records` selects a scalar FPR
`CallArgumentAbi` move such as `s13 -> s0`, and
`backend_aarch64_machine_printer` prints that selected move as
`fmov s0, s13`.

Fresh `00204.c` generated assembly now publishes the fixed HFA float lanes
before direct calls: the `fa_hfa11(hfa11)` path loads the lane through `s13`
and now emits `fmov s0, s13` immediately before `bl fa_hfa11`; the wider
`fa_hfa12`/`fa_hfa13`/`fa_hfa14` float-lane calls similarly publish into
`s0`/`s1`/`s2` as needed.

## Suggested Next

Next packet should investigate fixed HFA return classification/publication on
AArch64. Fresh generated BIR still lowers functions such as `fr_hfa11` as
`ptr sret(size=4, align=4)` and generated assembly uses stack/pointer return
storage instead of publishing HFA return lanes through ABI FPRs such as `s0`.

## Watchouts

- Do not reopen global initializer lowering unless fresh evidence shows missing
  BIR initializer data. Current generated assembly proves the AArch64 emitter
  now materializes `F32`/`F64` HFA global data.
- The direct fixed-HFA argument path now emits scalar FPR publication before
  `bl`; reopen it only if a fresh generated callsite again reaches a fixed HFA
  callee without `s`/`d` ABI FPR publication.
- The remaining visible HFA-family bad fact is not the fixed argument call
  lane: fixed HFA returns such as `fr_hfa11` are still represented as sret
  pointer returns in BIR/generated assembly.
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
  `RUNTIME_NONZERO`/segmentation fault. The delegated CTest capture reported
  no stdout/stderr payload for the failing run, so use generated BIR/assembly
  facts rather than output text alone for the next first-bad-fact pass.
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
`RUNTIME_NONZERO`/segmentation fault after the fixed-HFA call-lane publication
repair; fresh generated assembly confirms `fmov s0, s13` before
`bl fa_hfa11`.

Proof log: `test_after.log`.
