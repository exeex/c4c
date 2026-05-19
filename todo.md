Status: Active
Source Idea Path: ideas/open/323_aarch64_vararg_consumption_source_progression.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Vararg Consumption Fault

# Current Packet

## Just Finished

Step 1: Localize Vararg Consumption Fault.

Fresh focused proof still fails only in
`c_testsuite_aarch64_backend_src_00204_c`, now after `myprintf` reaches the
post-`va_start` vararg consumers. The first bad consumer family is the HFA
`llvm.va_arg.aggregate` path in `myprintf`, beginning with the `%hfa11` branch
and continuing through float, double, and long-double HFA aggregate consumers.

Bad record/path:

- Semantic BIR for `myprintf` expands the initial `%7s`/`%9s` aggregate
  consumers into explicit stack/register branches, but HFA consumers remain as
  `llvm.va_arg.aggregate(ptr sret(...), ptr %lv.ap)` helper calls.
- Prepared/machine lowering selects
  `VariadicAggregateVaArgRecord` for those helpers.
- `src/backend/prealloc/variadic_entry_plans.cpp::make_aapcs64_aggregate_va_arg_access_plan()`
  hard-codes every aggregate helper to
  `source_class=OverflowArgArea`, `source_field=OverflowArgArea`, and
  `progression_field=OverflowArgArea`.
- `src/backend/mir/aarch64/codegen/variadic.cpp::print_aggregate_va_arg_lowering_lines()`
  emits executable chunk copies through
  `append_aggregate_source_address_lines()` and then advances the configured
  progression field. The generated code therefore copies from `[x21]`
  (`__stack`) and advances `[x21]` even for HFA/floating/long-double aggregate
  varargs that should be selected from the initialized FP register save area
  while FP register slots remain available.

Responsibility split for the repair:

- Source selection: prepared aggregate `va_arg` access planning must classify
  HFA/floating aggregate payloads as FP register-save-area candidates when the
  ABI says they are passed in FP/SIMD slots, falling back to overflow only when
  the `va_list` FP offset is exhausted.
- Value transport: the AArch64 aggregate printer already emits executable
  load/store chunks, but it needs the right source class, field, slot stride,
  payload offsets, and HFA lane layout. For register-save-area HFA payloads,
  copies must read each lane from the low bytes of consecutive FP register-save
  slots, not from the contiguous overflow payload address.
- `va_list` progression: register-save-area HFA consumers must advance
  `FpOffset` by the number of consumed FP slots times the FP slot size; overflow
  consumers must continue to advance `OverflowArgArea` by the aligned payload
  stride. The current aggregate path always advances the overflow pointer.

Owning code surfaces:

- `src/backend/prealloc/variadic_entry_plans.cpp` aggregate access-plan source
  classification, source slot/payload offsets, and progression field/stride.
- `src/backend/prealloc/variadic.hpp` aggregate access-plan facts if the repair
  needs to publish HFA lane count or register-save-area copy shape.
- `src/backend/mir/aarch64/codegen/instruction.hpp` and
  `src/backend/mir/aarch64/codegen/variadic.cpp` if the machine record/printer
  needs branch-aware or lane-aware aggregate source/progression fields.
- Representative tests:
  `backend_lir_to_bir_notes`,
  `backend_aarch64_target_instruction_records`,
  `backend_aarch64_machine_printer`,
  `backend_aarch64_instruction_dispatch`,
  focused `00204` BIR/prepared-BIR dump tests, and
  `c_testsuite_aarch64_backend_src_00204_c`.

## Suggested Next

Execute Step 2 from `plan.md`: repair HFA aggregate `va_arg` source selection,
value transport, and progression so register-save-area HFA consumers read from
the FP save area while available and fall back to overflow according to AAPCS64.

## Watchouts

- The repaired `va_start` destination sequence from idea 322 must remain
  intact unless evidence proves it points at the wrong writable object.
- Raw `va.arg.aggregate*` text must remain absent.
- Do not treat all aggregate varargs as overflow-area consumers. That is the
  localized first bad fact for HFA/floating/long-double consumers.
- Do not replace the semantic BIR `%7s`/`%9s` branch-expanded consumers with a
  named-case shortcut; they are useful contrast evidence for source/progression
  branching.
- If Step 2 needs conditional register-save-area versus overflow selection, keep
  it general across HFA arity and element width rather than keyed to `%hfa11`,
  `%hfa34`, or `00204.c`.
- Do not special-case `00204.c`, `stdarg`, `myprintf`, one argument index, one
  type, one register, one stack slot, one offset, or one emitted instruction
  sequence.
- Do not weaken expectations, unsupported classifications, CTest registration,
  runner behavior, timeout policy, proof-log policy, or prior guardrails.
- Preserve the repairs from ideas 314, 315, 317, 318, 319, 320, 321, and 322.

## Proof

Reran the supervisor-delegated focused proof and wrote output to
`test_after.log`:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)|backend_cli_dump_(bir|prepared_bir)_(00204_stdarg_(semantic|prepared)_handoff|focus_(function_filters|block_entry)_00204)|c_testsuite_aarch64_backend_src_00204_c)$'`

Result: build succeeded; CTest ran 11 tests, 10 passed and 1 failed. Only
failure is `c_testsuite_aarch64_backend_src_00204_c`, still
`[RUNTIME_NONZERO] exit=Segmentation fault`. The log output reaches scalar and
string varargs, then corrupts HFA long-double/floating output. Generated
`build/c_testsuite_aarch64_backend/src/00204.c.s` has no raw
`va.arg.aggregate*` text, and `myprintf` still contains the repaired
`add x21, sp, #816` before the `va_start` stores.
