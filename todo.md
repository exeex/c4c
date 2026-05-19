Status: Active
Source Idea Path: ideas/open/321_aarch64_aggregate_va_arg_helper_lowering.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate And Classify Residuals

# Current Packet

## Just Finished

Step 4 classified the post-Step-2 runtime first bad fact without implementation
changes.

The aggregate `va_arg` helper owner is no longer the first blocker:
`build/c_testsuite_aarch64_backend/src/00204.c.s` contains no raw
`va.arg.aggregate`, `va.arg.aggregate.source`, or
`va.arg.aggregate.progress` text after Step 2, and the focused proof reaches
runtime.

The remaining `c_testsuite_aarch64_backend_src_00204_c` failure is a
segmentation fault caused by the generated `myprintf` `va_start` prologue
storing through `x21` before `x21` is materialized as the local `va_list`
address:

- `myprintf:` at generated assembly lines 4705 onward saves incoming `x21` with
  `str x21, [sp, #824]`.
- The `va_start` helper then emits `mov x0, x21`, `add x9, sp, #816`,
  `str x9, [x21]`, `str x9, [x21, #8]`, `str x9, [x21, #16]`,
  `str w9, [x21, #24]`, and `str w9, [x21, #28]`.
- There is no preceding `add x21, sp, #...`, reload from a valid frame slot, or
  other publication of a writable local `va_list` address in `myprintf` before
  those stores.

The identifiable owning code surface is the AArch64 `VaStart` path in
`src/backend/mir/aarch64/codegen/variadic.cpp`:
`make_variadic_va_start_record()` captures `homes.destination_va_list`, and
`print_va_start_lowering_lines()` currently requires a register destination and
uses that register directly as the base for `va_list` field stores. The
aggregate helper code added for idea 321 consumes the same `va_list` later, but
it does not own publishing the initial `va_list` object address.

Classification: the remaining segfault is outside idea 321's aggregate
`va_arg` source/copy/progression lowering owner. It should be split to a
separate lifecycle owner for AArch64 `va_start` destination address
materialization / frame-slot publication so the prepared `va_list` destination
is a real writable local object before `VaStart` field stores run.

## Suggested Next

Ask the plan owner to close or retire the completed idea 321 runbook and create
/ activate a follow-up idea for AArch64 `va_start` destination address
materialization / frame-slot publication.

## Watchouts

- `rg 'va\\.arg\\.aggregate' build/c_testsuite_aarch64_backend/src/00204.c.s`
  returns no matches after this packet.
- The external representative now reaches runtime, so the prior assembler
  blocker is gone.
- The aggregate helper lowering intentionally does not special-case `00204.c`,
  one payload size, one register, one stack slot, or one emitted line.
- The local aggregate helper test covers executable output but uses a small
  stack destination; the external focused representative exercised large stack
  offsets in generated assembly.
- Do not reopen F128 transport addressability; that owner is closed.
- Do not fold the new `va_start` destination-address fault into aggregate
  `va_arg` helper lowering; the failing stores are emitted before aggregate
  `va_arg` source/copy/progression executes.
- Do not special-case `00204.c`, `stdarg`, `myprintf`, one HFA struct, one
  register, one stack slot, one helper index, or one emitted line.
- Do not weaken expectations, unsupported classifications, CTest registration,
  runner behavior, timeout policy, proof-log policy, or prior guardrails.
- Preserve the repairs from ideas 314, 315, 317, 318, 319, and 320.
- Keep global initializer emission separate unless it becomes the first bad
  fact after aggregate `va_arg` helper text is gone.

## Proof

Fresh Step 4 proof was written to `test_after.log`:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)|backend_cli_dump_(bir|prepared_bir)_(00204_stdarg_(semantic|prepared)_handoff|focus_(function_filters|block_entry)_00204)|c_testsuite_aarch64_backend_src_00204_c)$'`

Result: build succeeded; CTest ran 11 tests, 10 passed, and only
`c_testsuite_aarch64_backend_src_00204_c` failed with
`[RUNTIME_NONZERO] exit=Segmentation fault`.

The focused backend checks, including `backend_aarch64_machine_printer`, passed.
The generated representative assembly no longer contains raw
`va.arg.aggregate`, `va.arg.aggregate.source`, or
`va.arg.aggregate.progress` text. The residual is classified as outside the
aggregate `va_arg` helper owner.
