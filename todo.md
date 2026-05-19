Status: Active
Source Idea Path: ideas/open/313_aarch64_f128_transport_machine_printer.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate And Classify Residuals

# Current Packet

## Just Finished

Step 4 localized and classified the post-f128 `00204.c` residual. The old
`f128_transport` selected-but-unprintable diagnostic remains gone; the fresh
first-bad fact is now:
`function 0 block 6 instruction 0: cannot print AArch64 machine node
family=scalar opcode=add: scalar ALU stack publication offset is not
printable`.

The owning code surface is scalar stack-result publication, not f128
transport. `lower_scalar_instruction` in
`src/backend/mir/aarch64/codegen/alu.cpp` can form a selected scalar
`add` publication node for a stack-backed scalar result after
`make_prepared_scalar_result_operand` records
`result_stack_offset_bytes` from prepared frame-slot storage. The printer then
routes through `append_scalar_alu_stack_publication` and
`scalar_alu_stack_publication_line`, which currently emits a direct
`str <result>, [sp, #offset]` and rejects offsets outside `0..4095`.

Classification: this is outside idea 313. Idea 313 should be considered
complete from the executor side because focused f128 backend tests pass and
`00204.c` advanced past the f128 transport printer diagnostic. The remaining
residual is a large stack-slot addressing/publication issue and should be
handed to plan-owner disposition for idea 314 or a closely related split under
the large stack-offset owner.

## Suggested Next

Ask the plan owner to close or retire idea 313's active plan as complete, then
route the scalar stack-publication residual to the large stack-offset
addressing owner. The smallest next repair surface should include focused
AArch64 scalar ALU printer/dispatch coverage for a stack-backed scalar result
whose prepared frame-slot offset exceeds the direct store immediate range,
without matching `00204.c`, `stdarg`, block numbers, instruction numbers, or a
single offset literal.

## Watchouts

- Do not keep this residual in idea 313 merely because it appears in the same
  `00204.c` representative; idea 313's scope is structured `f128_transport`.
- The scalar failure likely shares the large-offset addressing family with
  idea 314, but the plan owner should decide whether to fold scalar
  stack-publication into idea 314 or split a sibling owner.
- Do not match on `00204.c`, `stdarg`, function/block/instruction numbers, or
  diagnostic strings.
- Do not change expectations, unsupported classifications, allowlists, runner
  behavior, timeout policy, proof-log policy, CTest registration, or semantic
  handoff contracts.

## Proof

Ran:
`cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records|backend_lir_to_bir_notes|backend_cli_dump_bir_00204_stdarg_semantic_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff|backend_cli_dump_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_block_entry_00204|c_testsuite_aarch64_backend_src_00204_c)$' > test_after.log 2>&1`

Result: build succeeded with no work to do. CTest ran 10 tests; 9 passed,
including `backend_aarch64_machine_printer`,
`backend_aarch64_instruction_dispatch`, and
`backend_aarch64_target_instruction_records`.
`c_testsuite_aarch64_backend_src_00204_c` failed with the classified scalar
stack-publication diagnostic above. Proof log: `test_after.log`.
