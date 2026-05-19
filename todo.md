Status: Active
Source Idea Path: ideas/open/324_aarch64_variadic_frame_formal_publication.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate And Classify Residuals

# Current Packet

## Just Finished

Step 4 reran the focused proof and classified the remaining post-Step-2
runtime residual for idea 324.

The original frame/formal publication blockers are repaired in the refreshed
generated artifact `build/c_testsuite_aarch64_backend/src/00204.c.s`:

- `myprintf` allocates `1408` bytes, and the prior `myprintf` out-of-frame
  access family such as `[sp, #9696]` is absent.
- `%p.format` is published before variadic setup with
  `str x0, [sp, #624]`.
- `myprintf` no longer emits the bad entry `mov x0, x21`.
- `va_start` still materializes a writable local `va_list` at
  `add x21, sp, #1328` before field stores through `[x21]`.
- The variadic register-save area begins at `stack+1136`, so it no longer
  overlaps `%p.format` at `stack+624`.
- Raw `va.arg.aggregate*` helper text remains absent.
- Aggregate/floating consumers still use executable source selection and
  `FpOffset` progression through `[x21, #28]` with overflow fallback labels.

The remaining `c_testsuite_aarch64_backend_src_00204_c` failure is still
`[RUNTIME_NONZERO] exit=Segmentation fault`, but the first generated-output
evidence no longer points at idea 324's frame-size or fixed-formal publication
owner. Instead, `myprintf` now reads ordinary local/value homes that are not
published in the current function before first use:

- The format cursor is seeded with a pointer to `%p.format` via
  `add x9, sp, #624; str x9, [sp]`, but the loop immediately tests
  `cmp w13, #0` without a preceding local load that defines `w13`.
- The first matcher path reads pattern and printf operands from local homes
  such as `[sp, #640]`, `[sp, #648]`, `[sp, #656]`, `[sp, #664]`,
  `[sp, #672]`, `[sp, #680]`, `[sp, #688]`, `[sp, #704]`, and `[sp, #712]`
  without same-function stores to those homes in `myprintf`.
- Scalar/overflow state for some vararg paths reads homes such as
  `[sp, #520]` and `[sp, #632]` before a same-function initialization store.

Classification: the residual is outside idea 324 as originally scoped. Frame
coverage, fixed-formal publication, `va_start` destination materialization,
raw-helper lowering, and HFA/aggregate `va_arg` source/progression remain
intact. The next owner should be AArch64 ordinary local/value-home
initialization and constant/pattern operand publication in variadic functions,
with emphasis on publishing local homes before generated control-flow and
matcher/printf operands consume them.

## Suggested Next

Ask the plan owner to close or retire idea 324 and activate a follow-up owner
for AArch64 local/value-home initialization and constant/pattern operand
publication in variadic functions.

## Watchouts

- Preserve prior repairs: large stack offsets, large frame adjustments,
  `va_start` helper lowering, scalar ALU immediates, HFA argument lanes, F128
  transport, aggregate helper text lowering, `va_start` destination
  materialization, and aggregate/floating `va_arg` source/progression.
- Raw `va.arg.aggregate*` text must stay absent.
- HFA/floating aggregate consumers should keep FP register-save-area source
  selection with `FpOffset` progression and overflow fallback unless generated
  evidence proves that path still owns the first bad fact.
- The next owner should not reopen frame sizing or fixed-formal publication
  unless fresh generated output again shows an uncovered stack reference or a
  fixed formal consumed before publication.
- The current local-home evidence is not a license to special-case
  `myprintf`, format strings, stack offsets, or `00204.c`; repair should be a
  general AArch64 local/value publication rule.
- Do not special-case `00204.c`, `stdarg`, `myprintf`, `format`, `x0`, `x21`,
  one local, one stack slot, one offset, or one emitted instruction sequence.
- Do not weaken expectations, unsupported classifications, CTest registration,
  runner behavior, timeout policy, proof-log policy, or prior guardrails.
- The current generated `stdarg` function contains large temporary HFA/vector
  stack traffic, but its prologue uses a large register-mediated frame
  adjustment that covers the observed offsets. Do not treat those large offsets
  as the old frame-size bug without checking the function-local prologue.
- The smallest representative local surfaces are prepared-BIR/function summary
  dumps for `00204.c`, `backend_aarch64_machine_printer`,
  `backend_aarch64_target_instruction_records`, and
  `backend_aarch64_instruction_dispatch`.

## Proof

Ran the delegated Step 4 focused proof into `test_after.log`:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)|backend_cli_dump_(bir|prepared_bir)_(00204_stdarg_(semantic|prepared)_handoff|focus_(function_filters|block_entry)_00204)|c_testsuite_aarch64_backend_src_00204_c)$'`

Build succeeded. CTest ran 11 tests: 10 passed, 1 failed. Internal focused
backend tests passed. The only failure is
`c_testsuite_aarch64_backend_src_00204_c`, still
`[RUNTIME_NONZERO] exit=Segmentation fault`.

Proof log path: `test_after.log`.
