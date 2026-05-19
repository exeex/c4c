Status: Active
Source Idea Path: ideas/open/315_aarch64_large_frame_adjustment_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Frame Adjustment Paths

# Current Packet

## Just Finished

Step 1 - Localize Frame Adjustment Paths is complete.

The missing fact is general large frame-adjustment materialization for
`FrameInstructionRecord` setup/teardown nodes. The current representative
`00204.c` failure reaches the AArch64 machine printer with
`family=frame opcode=frame_setup` and `frame_size=5776`, then rejects the node
because `print_frame` only accepts the plain `sub/add sp, sp, #imm` immediate
range `0..4095`.

Owning surfaces:

- Setup and teardown nodes are synthesized together in
  `src/backend/mir/aarch64/codegen/prologue.cpp` by
  `insert_prepared_frame_boundary_nodes` through
  `make_frame_machine_instruction`.
- The immediate rejection and direct in-range spelling live in
  `src/backend/mir/aarch64/codegen/machine_printer.cpp::print_frame`, which
  emits setup as `sub sp, sp, #frame_size` and teardown as
  `add sp, sp, #frame_size`.
- The target record/dispatch identity surfaces are
  `FrameInstructionRecord`, `make_frame_instruction`, and the existing
  `FrameSetup` / `FrameTeardown` dispatch records.

Representative coverage for the repair:

- `tests/backend/mir/backend_aarch64_machine_printer_test.cpp` has the direct
  in-range frame setup/teardown printer guard and should gain large setup plus
  large teardown materialization checks.
- `tests/backend/mir/backend_aarch64_target_instruction_records_test.cpp`
  covers frame record identity/effects and should keep setup/teardown records
  selected for large frame sizes.
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp` covers
  prepared module lowering around the function stream and should remain a
  provenance/ordering guard.
- `c_testsuite_aarch64_backend_src_00204_c` remains the external
  representative for `frame_size=5776`.

## Suggested Next

Execute Step 2 of the active plan. Repair
`src/backend/mir/aarch64/codegen/machine_printer.cpp::print_frame` so
`FrameInstructionKind::PrologueSetup` and
`FrameInstructionKind::EpilogueTeardown` materialize frame adjustments larger
than the plain immediate range through legal AArch64 stack-pointer sequences,
while keeping encodable frame sizes on the existing direct `sub/add sp, sp,
#imm` path.

## Watchouts

- Do not fold idea 316's `00216.c` frame-size/slot-offset mismatch into this
  owner.
- Do not reopen idea 314's stack-slot load/store or scalar stack-publication
  instruction-spelling paths unless fresh evidence proves the frame adjustment
  residual depends on them.
- Preserve direct frame setup/teardown output for encodable frame sizes.
- Repair setup and teardown together so the stack pointer remains balanced.
- The first local implementation target is the printer surface; do not widen
  into frame layout unless generated-code evidence proves `frame_size=5776` is
  wrong.
- No implementation files, tests, expectations, unsupported classifications,
  runners, timeout policy, or CTest registration were changed in this packet.

## Proof

Focused proof command for this owner:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)|backend_cli_dump_(bir|prepared_bir)_(00204_stdarg_(semantic|prepared)_handoff|focus_(function_filters|block_entry)_00204)|c_testsuite_aarch64_backend_src_00204_c)$'
```

Fresh proof was run with this exact command and written to `test_after.log`.
Result: build succeeded; 11 tests ran; 10 passed; only
`c_testsuite_aarch64_backend_src_00204_c` failed at the unchanged
`frame_setup` immediate materialization diagnostic.
