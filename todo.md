Status: Active
Source Idea Path: ideas/open/323_aarch64_vararg_consumption_source_progression.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Consumer Source And Progression

# Current Packet

## Just Finished

Step 2: Repair Consumer Source And Progression.

Implemented a prepared HFA/floating aggregate `va_arg` access shape for
AArch64:

- `make_aapcs64_aggregate_va_arg_access_plan()` now detects HFA aggregate
  helpers from the immediate FP lane loads following the sret aggregate helper
  call. Integer/memory aggregate helpers remain overflow-area consumers.
- `PreparedVariadicAggregateVaArgAccessPlan` now carries helper block/inst
  identity plus register-save-area lane count and lane size facts for machine
  lowering.
- `print_aggregate_va_arg_lowering_lines()` now emits executable
  register-save-area versus overflow branching. HFA/floating aggregates read
  lanes from consecutive FP register-save slots while `FpOffset + stride <= 0`,
  copy each lane’s low bytes into the aggregate payload home, and advance
  `FpOffset` by `lane_count * fp_slot_size`. Overflow fallback still copies the
  contiguous payload and advances `OverflowArgArea`.
- `va_start` lowering now also populates the AAPCS64 GP and FP/SIMD register
  save area slots for unnamed incoming argument registers before publishing the
  `va_list` field pointers and offsets.
- Focused backend coverage now proves the selected aggregate helpers expose
  `source_class=register_save_area`, `source_field=fp_register_save_area`,
  `progression_field=fp_offset`, and lane metadata for the HFA/floating
  aggregate consumers in `myprintf`.

## Suggested Next

Localize the remaining external runtime segfault after Step 2. The generated
`00204.c.s` now has HFA aggregate `va_arg` FP-register-save-area branches and
no raw `va.arg.aggregate*` text, but the c-testsuite case still exits with a
segmentation fault before producing buffered stdout. The next packet should
identify the first bad fact in the generated `myprintf` entry/loop state rather
than changing aggregate `va_arg` source selection again.

## Watchouts

- The repaired `va_start` destination sequence from idea 322 must remain
  intact unless evidence proves it points at the wrong writable object.
- Raw `va.arg.aggregate*` text must remain absent.
- Do not regress the new aggregate helper contract: HFA/floating aggregate
  consumers must keep using FP register-save-area slots with `FpOffset`
  progression and overflow fallback.
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

Reran the supervisor-delegated focused proof and wrote output to
`test_after.log`:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)|backend_cli_dump_(bir|prepared_bir)_(00204_stdarg_(semantic|prepared)_handoff|focus_(function_filters|block_entry)_00204)|c_testsuite_aarch64_backend_src_00204_c)$'`

Result: build succeeded; CTest ran 11 tests, 10 passed and 1 failed. The
focused backend/internal tests all pass, including
`backend_aarch64_machine_printer`,
`backend_aarch64_target_instruction_records`,
`backend_aarch64_instruction_dispatch`, and the focused BIR/prepared-BIR dump
checks. The only remaining failure is
`c_testsuite_aarch64_backend_src_00204_c`, still
`[RUNTIME_NONZERO] exit=Segmentation fault`.

Generated `build/c_testsuite_aarch64_backend/src/00204.c.s` has no raw
`va.arg.aggregate*` text. It now contains the repaired GP/FP register-save-area
stores in `va_start` and per-helper `.Lva_arg_aggregate_*` branches that test
`FpOffset`, copy HFA lanes from FP save slots, advance `FpOffset`, and fall
back to overflow copies when the FP save area is exhausted.
