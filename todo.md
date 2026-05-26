Status: Active
Source Idea Path: ideas/open/12_dispatch_value_materialization_authority.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Add or Tighten Focused Materialization Coverage

# Current Packet

## Just Finished

Step 5 coverage audit packet complete. No focused backend coverage gap was
found, so no tests were added or tightened.

Coverage map for the Step 5 target families:

- Prepared-value-home publication through the narrow bridge:
  `backend_aarch64_instruction_dispatch` covers fixed-formal, local-slot,
  call-result, HFA, branch-condition, stack-home, and prior-preservation
  publication paths feeding later consumers; `backend_aarch64_call_boundary_owner`
  covers structured call-result stack-home publication and preservation
  republication records.
- FP materialization subpaths that intentionally still consume the generic
  publication bridge: `backend_aarch64_instruction_dispatch` covers F64 ALU
  FPR facts, F64 select publication before global store and call argument
  moves, FP compare result publication, and conversion-cast store-source
  publication; `backend_aarch64_prepared_scalar_alu_records`,
  `backend_aarch64_prepared_scalar_cast_records`, and
  `backend_aarch64_machine_printer` cover the structured scalar/FPR/cast
  records and selected printing surfaces.
- Address/global materialization feeding memory, scalar, and branch routing:
  `backend_aarch64_prepared_memory_operand_records`,
  `backend_aarch64_instruction_dispatch`,
  `backend_codegen_route_aarch64_pointer_value_named_scalar_writeback_uses_computed_store_value`,
  `backend_codegen_route_aarch64_global_function_pointer_table_selected_indirect_call`,
  and
  `backend_codegen_route_aarch64_local_aggregate_address_pointer_copy_publishes_frame_address`
  cover pointer-value stores, computed/global address publication, local
  aggregate frame-address publication, global-load-to-branch materialization,
  and selected indirect-call address use.
- Call-boundary materialization handoffs through `calls_dispatch_bridge.*`:
  `backend_aarch64_call_boundary_owner`,
  `backend_codegen_route_aarch64_prepared_call_boundary_scalability`, and
  `backend_aarch64_instruction_dispatch` cover before-call, after-call,
  byval/aggregate, immediate, frame-slot, symbol-address, local-address, and
  preservation handoffs without reopening generic dispatch ownership.
- Publication, fused-compare, and condition routes delegated through
  `dispatch_publication.*`: `backend_aarch64_prepared_branch_records`,
  `backend_aarch64_instruction_dispatch`,
  `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`,
  and `backend_aarch64_machine_printer` cover materialized-bool facts,
  fused-compare facts, branch-condition stack homes, compare/select
  publication, join transfer/select materialization, and selected branch
  printing.

No-gap finding: the existing focused subset already combines semantic prepared
facts, machine-instruction records, and route-level publication/order checks
for every Step 2-4 ownership boundary family. The remaining text-snippet route
tests are representative end-to-end guards paired with structured record tests;
no unsupported downgrade, expectation weakening, or named-case shortcut was
needed.

## Suggested Next

Step 6 validation packet: run the supervisor-selected broader backend
validation for dispatch materialization cleanup and review the final diff for
hidden ownership drift, expectation weakening, or testcase-shaped coverage.

## Watchouts

- Keep this route focused on value materialization authority.
- Do not fold in edge-copy cleanup, publication cleanup, calls cleanup, or
  broad AArch64 pipeline redesign.
- Treat expectation weakening, unsupported downgrades, text-emission contracts,
  and named-test shortcuts as route failures.
- Leave unrelated transient `review/` artifacts untouched.
- `emit_value_publication_to_register` is the fan-out point and should not be
  the first move; it recursively consumes most leaf materializers and is used by
  calls, stores, edge copies, publication, and branch fusion hooks.
- The generic dispatcher now calls
  `emit_prepared_value_home_publication_to_register` as the narrow bridge into
  the new owner. Keep the two plan helpers private unless a later packet has a
  concrete direct client.
- The branch-fusion hook struct still carries a function pointer named
  `emit_prepared_value_home_to_register`; the function declaration itself now
  comes from `prepared_value_home_materialization.hpp`.
- `fp_value_materialization.cpp` still depends on generic
  `emit_value_publication_to_register` for integer compare/select and cast
  materialization subpaths; keep that bridge intentional unless a later packet
  splits those dependencies semantically.
- The local dispatch ordering bridges around address materialization are
  intentionally order-sensitive. Do not move them casually into globals, memory,
  or ALU without a separate packet that owns cross-owner ordering and proof.
- Step 5 should prefer semantic machine-instruction records and prepared facts
  over text-emission expectations.
- Do not add one-off tests keyed to a single BIR function name or exact
  assembly spelling when a structured machine-instruction or prepared-fact
  assertion can cover the same materialization path.
- Do not weaken, delete, or mark unsupported any existing expectation to claim
  Step 5 progress.
- The Step 5 audit found sufficient existing coverage, but several route tests
  remain text-snippet guards by nature; keep relying on the paired structured
  record tests when evaluating ownership-boundary proof.

## Proof

Command run exactly:
`cmake --build --preset default > test_after.log 2>&1` followed by
`ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_prepared_call_boundary_scalability|backend_codegen_route_aarch64_global_function_pointer_table_selected_indirect_call|backend_codegen_route_aarch64_pointer_value_named_scalar_writeback_uses_computed_store_value|backend_codegen_route_aarch64_scalar_fp_literal_add_publishes_fpr_result|backend_codegen_route_aarch64_local_aggregate_address_pointer_copy_publishes_frame_address|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication|backend_aarch64_instruction_dispatch|backend_aarch64_machine_printer|backend_aarch64_target_instruction_records|backend_aarch64_prepared_memory_operand_records|backend_aarch64_prepared_scalar_alu_records|backend_aarch64_prepared_scalar_cast_records|backend_aarch64_prepared_branch_records|backend_aarch64_call_boundary_owner)$' >> test_after.log 2>&1`

Result: passed. `test_after.log` contains the preserved proof log: build was
up to date and 14/14 delegated tests passed.
