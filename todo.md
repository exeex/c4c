Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate And Classify Residuals

# Current Packet

## Just Finished

Step 2 repaired the representable fixed HFA return path for AArch64 scalar FP
lanes. Semantic LIR-to-BIR return classification now recognizes homogeneous
`float`/`double` aggregates of 1..4 lanes and classifies them as FP register
returns instead of `ptr sret`; fixed HFA call results now carry explicit
per-lane BIR values, aggregate stores materialize each returned lane into the
matching local aggregate leaf, and aggregate returns load/publish every return
lane instead of collapsing to one scalar.

AArch64 return/call-result publication now covers scalar FPR HFA lanes:
callee-side before-return moves publish return lanes into `sN`/`dN`, after-call
moves consume call-result lanes from ABI FPRs, and the return printer preserves
the correct source/destination register views for scalar register publication.

Focused backend coverage now proves the classification/publication contracts:
`backend_lir_to_bir_notes` covers fixed HFA return and call-result
classification plus multi-lane aggregate materialization as scalar FP ABI lanes
rather than sret,
`backend_aarch64_target_instruction_records` selects scalar FPR
`FunctionReturnAbi` moves, and `backend_aarch64_machine_printer` prints scalar
FPR register returns with ABI publication such as `fmov s0, s13`.

## Suggested Next

Advance to Step 4 residual classification. `00204.c` now advances past
semantic LIR-to-BIR and backend assembly/linking. The current first bad fact is
runtime execution: the linked AArch64 backend binary exits with
`Segmentation fault` before producing output. Next packet should classify the
generated-code/runtime fault first, then decide whether it remains under idea
326's HFA/floating residual scope or needs a separate open idea. Do not reopen
fixed global initializer emission, HFA argument lanes, or HFA return
materialization unless fresh evidence points back to those owners.

## Watchouts

- Do not reopen global initializer lowering unless fresh evidence shows missing
  BIR initializer data. Current generated assembly proves the AArch64 emitter
  now materializes `F32`/`F64` HFA global data.
- The direct fixed-HFA argument path now emits scalar FPR publication before
  `bl`; reopen it only if a fresh generated callsite again reaches a fixed HFA
  callee without `s`/`d` ABI FPR publication.
- The previous fixed HFA return bad fact is repaired: `CallInst` carries
  explicit multi-lane result values and `ReturnTerminator` carries explicit
  return lanes for fixed `float`/`double` HFAs. Do not regress this back to
  one scalar result duplicated into multiple leaves.
- Review follow-up `review/326_hfa_return_step2_review.md` is addressed:
  after-call HFA call-result lane moves now have matching per-lane ABI
  bindings, so lane 1+ consumes `s1`/`d1` and later ABI FPRs instead of the
  primary `s0`/`d0` lane.
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

Result: build succeeded; 3/4 delegated CTest tests passed. The focused backend
tests `backend_aarch64_machine_printer`,
`backend_aarch64_instruction_dispatch`, and
`backend_aarch64_target_instruction_records` passed. The representative
`c_testsuite_aarch64_backend_src_00204_c` advanced past semantic
`FRONTEND_FAIL` and backend assembly/linking; it now fails as
`RUNTIME_NONZERO` with `exit=Segmentation fault`.

Additional focused check: `ctest --test-dir build -j --output-on-failure -R
'^backend_lir_to_bir_notes$'` passed after adding multi-lane HFA return/call
result materialization coverage.

Review follow-up checks:
`ctest --test-dir build -j --output-on-failure -R
'^(backend_prepare_liveness|backend_aarch64_instruction_dispatch)$'` passed,
including focused coverage that generated AArch64 HFA call-result lane
bindings publish `s0`/`s1` and dispatch consumes lane moves as `s0 -> s13`
and `s1 -> s14`. Broad backend validation
`ctest --test-dir build -j --output-on-failure -R '^backend_'` passed
140/140.

Proof log: `test_after.log`.
