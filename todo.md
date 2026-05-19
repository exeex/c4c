Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize HFA/Floating First Bad Fact

# Current Packet

## Just Finished

Step 1 localized the first `00204.c` HFA/floating bad fact. The direct
`fa_hfa11(hfa11)` path is already corrupted before the call-boundary consume:
`arg` loads `hfa11` from the generated data label, stores/reloads it through
local float homes, then calls `fa_hfa11`; `fa_hfa11` saves `s0`, reloads it,
converts it with `fcvt d8, s13`, and prints the received lane. The bad value is
therefore not first lost in `fa_hfa11`'s HFA argument consumer.

The first bad record/path is the global float HFA object itself:
`build/c_testsuite_aarch64_backend/src/00204.c.s` emits `hfa11:`,
`hfa12:`, `hfa13:`, `hfa14:`, `hfa21:` through `hfa24:` with only
`# global data emission deferred to behavior-recovery packet`, while `hfa31`
and later long-double globals do emit bytes. Prepared output still records
global-symbol loads from `hfa11` offset 0, including `arg`/`fa_hfa11`,
`fr_hfa11`, and later variadic uses, so the generated load path is reading
zero/uninitialized data from a missing F32/F64 global initializer emission.

Owning code surfaces for Step 2 are AArch64 global data emission in
`src/backend/mir/aarch64/codegen/asm_emitter.cpp`
(`global_initializer_directive`, `emit_global_initializer`,
`is_supported_scalar_global`, and aggregate initializer element emission) and,
only if evidence requires it, BIR global initializer lowering in
`src/backend/bir/lir_to_bir/global_initializers.cpp` /
`src/backend/bir/lir_to_bir/globals.cpp`. The observed generated-code failure
points first at the AArch64 emitter rejecting/deferring `F32`/`F64`
initializer values, not at variadic register-save-area progression or HFA
call-lane lowering.

## Suggested Next

Step 2 should repair AArch64 data emission for floating global initializers
generally: emit correct bytes/directives for `F32`/`F64` scalar values and for
aggregate initializer elements containing those types, then verify the first
`hfa11` bytes materialize as the source value for direct calls, returns, and
later variadic HFA loads.

Smallest focused repair proof recommendation:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records|c_testsuite_aarch64_backend_src_00204_c)$'`

## Watchouts

- Do not treat the current `fa_hfa11(hfa11)` failure as proof that
  register-save-area, overflow-area, `va_arg`, or HFA call-lane lowering is
  wrong. The first observed bad fact is earlier: missing emitted data for
  floating HFA globals.
- The same missing-data surface covers `hfa11` through `hfa24` (`float` and
  `double` HFA globals). Long-double HFA globals already emit raw bytes, so the
  Step 2 change should be type-general without regressing FP80/F128-style byte
  emission.
- Representative tests for the repair are
  `backend_aarch64_machine_printer`,
  `backend_aarch64_instruction_dispatch`,
  `backend_aarch64_target_instruction_records`, and
  `c_testsuite_aarch64_backend_src_00204_c`; add focused local coverage around
  AArch64 F32/F64 global initializer emission before relying only on the
  external representative.
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
- The remaining runtime representative failure is no longer a timeout. The
  CTest case exits with `RUNTIME_NONZERO`/segmentation fault after printing
  substantial output. The next first bad fact is the first HFA float argument
  output: `fa_hfa11(hfa11)` prints `0.0` instead of `11.1`.
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
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_cli_dump_bir_00204_stdarg_semantic_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication|backend_cli_dump_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_block_entry_00204|backend_cli_dump_prepared_bir_focus_block_entry_00204|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records|c_testsuite_aarch64_backend_src_00204_c)$'`

Result: build was up to date; 9/10 focused tests passed. The sole failing test
remains `c_testsuite_aarch64_backend_src_00204_c` with
`RUNTIME_NONZERO`/segmentation fault and the known first bad output where
`fa_hfa11(hfa11)` prints `0.0` instead of `11.1`.

Proof log: `test_after.log`.
