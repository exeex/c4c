Status: Active
Source Idea Path: ideas/open/323_aarch64_vararg_consumption_source_progression.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate And Classify Residuals

# Current Packet

## Just Finished

Step 4: Validate And Classify Residuals.

Reran the focused proof after Step 2 and classified the remaining
`c_testsuite_aarch64_backend_src_00204_c` segfault.

The original idea 323 first bad fact is repaired:

- Generated `build/c_testsuite_aarch64_backend/src/00204.c.s` has no raw
  `va.arg.aggregate*` helper text.
- `myprintf` still materializes the writable `va_list` destination before
  field publication: `add x21, sp, #816`, followed by GP/FP register-save-area
  stores and `va_list` field stores through `[x21]`.
- The aggregate HFA/floating consumers still emit executable
  `.Lva_arg_aggregate_*` branches that test `FpOffset`, read lanes from the
  FP register-save area through `[x21, #16] + [x21, #28]`, advance
  `[x21, #28]`, and retain overflow fallbacks.

The remaining segfault is outside idea 323's aggregate `va_arg`
source/progression owner. The generated `myprintf` entry allocates only
`896` bytes (`sub sp, sp, #896`) but later accesses local/spill slots far past
that frame, for example `ldr x1, [sp, #9696]` while matching `%7s` and
`str w9, [sp, #9752]` in the first HFA consumer path. It also clobbers the
incoming format pointer before the loop with `mov x0, x21`; since `x21` is
only the saved callee register at that point, the subsequent `ldrb w13, [x0]`
does not reliably read the format string. Those are frame/local publication or
formal preservation faults, not aggregate `va_arg` source-selection or
progression faults.

## Suggested Next

Ask the plan owner to close or retire idea 323 as complete and activate a new
owner for AArch64 local/frame-slot publication and formal argument preservation
in variadic functions. The next implementation packet should localize why
`myprintf`'s frame allocation is smaller than the generated stack-slot offsets
and why the incoming `format` pointer is overwritten by `mov x0, x21` before
the format loop.

## Watchouts

- The repaired `va_start` destination sequence from idea 322 remains intact;
  do not route the new fault back to destination materialization without new
  evidence.
- Raw `va.arg.aggregate*` text remains absent and must stay absent.
- The new aggregate helper contract remains intact: HFA/floating aggregate
  consumers use FP register-save-area slots with `FpOffset` progression and
  overflow fallback.
- The residual accesses `myprintf` stack offsets above the allocated frame and
  corrupts the format pointer before the loop; a repair that only tweaks one
  `%hfa*` consumer would be testcase overfit.
- Do not replace the semantic BIR `%7s`/`%9s` branch-expanded consumers with a
  named-case shortcut; they are useful contrast evidence for source/progression
  branching.
- Do not special-case `00204.c`, `stdarg`, `myprintf`, one argument index, one
  type, one register, one stack slot, one offset, or one emitted instruction
  sequence.
- Do not weaken expectations, unsupported classifications, CTest registration,
  runner behavior, timeout policy, proof-log policy, or prior guardrails.
- Preserve the repairs from ideas 314, 315, 317, 318, 319, 320, 321, and 322.

## Proof

Reran the supervisor-delegated focused proof for Step 4 and wrote output to
`test_after.log`:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)|backend_cli_dump_(bir|prepared_bir)_(00204_stdarg_(semantic|prepared)_handoff|focus_(function_filters|block_entry)_00204)|c_testsuite_aarch64_backend_src_00204_c)$'`

Result: build succeeded; CTest ran 11 tests, 10 passed and 1 failed. The
focused backend/internal tests all pass, including
`backend_aarch64_machine_printer`,
`backend_aarch64_target_instruction_records`,
`backend_aarch64_instruction_dispatch`, and the focused BIR/prepared-BIR dump
checks. The only remaining failure is
`c_testsuite_aarch64_backend_src_00204_c`, still
`[RUNTIME_NONZERO] exit=Segmentation fault`, with empty captured
`stdout+stderr`.

Classification: no new raw-helper or aggregate HFA source/progression failure
was found. The first remaining generated-output fault is under-sized
`myprintf` frame/local slot publication plus incoming-format preservation.
