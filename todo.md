Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate And Classify Residuals

# Current Packet

## Just Finished

Step 4 classified the current `00204.c` AArch64 runtime segmentation fault.
The fault is not the next HFA/floating lane consume; it is a distinct adjacent
AArch64 function-entry formal publication issue for the ordinary fixed
variadic `format` pointer in `myprintf`.

Runtime evidence from LLDB: the binary reaches `stdarg:`, enters
`myprintf("%9s ...", ...)`, then stops at `myprintf+152` on
`ldrb w9, [x10]` with `x10 = 0`. At the same stop, `x0` still identifies the
format string `.str49`, and `lr = stdarg+1808`, immediately after the first
`myprintf` callsite in `stdarg`.

Generated-code evidence: the `stdarg` callsite materializes `.str49` into
`x0` before `bl myprintf`, but the `myprintf` prologue stores `x1`..`x7` and
`q0`..`q7`, initializes the variadic entry helper, then emits `str x13, [sp]`
for `bir.store_local %lv.s, ptr %p.format`. There is no preceding
`x0 -> x13` or `x0 -> [sp]` publication for `%p.format`. Prepared BIR confirms
the semantic handoff is correct (`bir.func @myprintf(ptr %p.format)` and
`bir.store_local %lv.s, ptr %p.format`), while the AArch64 prepared storage
assigns `%p.format` to `register:x13`; the first-use codegen then consumes the
unpublished `x13` value.

Classification: split this as adjacent to idea 326 before implementing a
repair. The visible HFA/floating output is still corrupt earlier in the run,
but the current fatal first bad fact is fixed-formal entry ABI publication for
a non-HFA pointer parameter in a variadic callee.

## Suggested Next

Smallest next packet: create or activate an adjacent AArch64 fixed-formal
entry-publication initiative, then add a focused prepared/codegen/runtime proof
that a function parameter assigned to non-ABI storage is populated from its
AAPCS64 incoming register before first use. The minimal representative should
cover a variadic callee with one fixed pointer formal, e.g. `myprintf(ptr
%format, ...)`, and then rerun the same `00204.c` focused proof to expose the
next HFA/floating bad fact after the null-deref is gone.

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
- Fresh generated output does show a fixed-formal entry publication gap:
  `%p.format` is assigned to `x13` in prepared AArch64 storage, while AAPCS64
  passes the first argument in `x0`; generated `myprintf` first stores/loads
  from the `x13`-derived local cursor and dereferences null.
- Suspected owning surfaces are AArch64 prepared value-home/regalloc handling
  for BIR function params and AArch64 codegen of function-entry parameter
  publication. Avoid treating this as an HFA `va_arg` source/progression repair
  unless a later post-segfault run reaches the HFA/floating path again.

## Proof

Delegated proof run:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_cli_dump_bir_00204_stdarg_semantic_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication|backend_cli_dump_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_block_entry_00204|backend_prepare_liveness|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records|c_testsuite_aarch64_backend_src_00204_c)$'`

Result: build succeeded; 11/12 delegated CTest tests passed. The only failure
is `c_testsuite_aarch64_backend_src_00204_c`, which still fails as
`RUNTIME_NONZERO` with `exit=Segmentation fault`. The backend/BIR/prologue
guardrails in the delegated subset passed, so the classification evidence comes
from generated assembly, prepared dump inspection, and LLDB runtime state.

Proof log: `test_after.log`.
