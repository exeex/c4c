Status: Active
Source Idea Path: ideas/open/324_aarch64_variadic_frame_formal_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Frame And Formal Publication Fault

# Current Packet

## Just Finished

Lifecycle reset from idea 323 closure. Step 1 should localize the AArch64
variadic frame/formal publication fault before implementation begins.

## Suggested Next

Run Step 1: inspect generated `00204.c` AArch64 artifacts and backend
frame/formal publication surfaces to determine why `myprintf` allocates only
`896` bytes while referencing offsets such as `[sp, #9696]`, and why the
incoming `format` pointer is overwritten by `mov x0, x21` before the format
loop.

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

## Proof

Lifecycle close gate for idea 323 used the existing focused
`test_before.log` / `test_after.log` scope:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)|backend_cli_dump_(bir|prepared_bir)_(00204_stdarg_(semantic|prepared)_handoff|focus_(function_filters|block_entry)_00204)|c_testsuite_aarch64_backend_src_00204_c)$'`

Regression guard passed in non-decreasing mode: 10 passed, 1 failed before and
after; no new failures. The remaining failure is
`c_testsuite_aarch64_backend_src_00204_c` at runtime segfault, classified to
idea 324.
