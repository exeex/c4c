Status: Active
Source Idea Path: ideas/open/315_aarch64_large_frame_adjustment_materialization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Large Frame Adjustment Materialization

# Current Packet

## Just Finished

Step 2 - Repair Large Frame Adjustment Materialization is complete.

`src/backend/mir/aarch64/codegen/machine_printer.cpp::print_frame` now keeps
encodable frame setup/teardown adjustments on the existing direct
`sub/add sp, sp, #imm` path and materializes larger frame sizes through the
reserved MIR scratch GPR before applying `sub/add sp, sp, x9`. Setup and
teardown use the same materialized frame-size fact so stack-pointer balance is
preserved.

The same frame-node printer surface now also materializes large frame-node
save/restore stack offsets for link-register and callee-save records through a
scratch stack address before emitting `str/ldr`, which was the next
representative frame-node fact exposed after repairing the setup immediate.

Focused local coverage was added in
`tests/backend/mir/backend_aarch64_machine_printer_test.cpp` for:

- direct in-range setup/teardown output remaining unchanged
- large setup plus matching teardown adjustment materialization
- large link-register frame slot save/restore address materialization

## Suggested Next

Execute Step 4 of the active plan. Classify the new `00204.c` representative
residuals now that the frame setup/teardown printer diagnostic is gone:
raw `va.start` note lines are reaching the assembler, and a later large
`mov w9, #503808` immediate is not legal AArch64 assembly. Treat them as
outside this frame-adjustment owner unless fresh generated-code evidence proves
otherwise.

## Watchouts

- Do not fold idea 316's `00216.c` frame-size/slot-offset mismatch into this
  owner.
- Do not reopen idea 314's stack-slot memory/scalar-publication paths from this
  slice; the remaining `mov w9, #503808` assembler residual is a separate
  scalar/immediate materialization fact unless classified otherwise.
- The raw `va.start` note assembly residual is not a frame setup/teardown
  adjustment fact.
- No `prologue.cpp`, plan/source-idea files, expectations, unsupported
  classifications, runners, timeout policy, CTest registration, or proof-log
  policy were changed in this packet.

## Proof

Focused proof command for this owner:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)|backend_cli_dump_(bir|prepared_bir)_(00204_stdarg_(semantic|prepared)_handoff|focus_(function_filters|block_entry)_00204)|c_testsuite_aarch64_backend_src_00204_c)$'
```

Fresh proof was run with this exact command and written to `test_after.log`.
Result: build succeeded; 11 tests ran; 10 passed. The old
`frame_setup` immediate materialization diagnostic is gone. The only failing
test is still `c_testsuite_aarch64_backend_src_00204_c`, now as a backend
assembler failure with new first bad facts:

- raw `va.start` note payload lines are emitted into the assembly stream
- a later `mov w9, #503808` uses an immediate that is not accepted by the
  assembler
