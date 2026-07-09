Status: Active
Source Idea Path: ideas/open/624_prepared_outgoing_stack_argument_destination_offsets.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Wire consumer guards without inference

# Current Packet

## Just Finished

Completed Step 4 consumer guard proof: RV64 text/object byval stack-copy call
consumers require prepared destination offset/size facts, matching transport
destination facts, and matching outgoing stack-area authority without deriving
destination offsets locally.

Artifact:
`build/agent_state/624_step4_consumer_guards/`

Result:

- Verified `emit_riscv_byval_aggregate_address_argument(...)` and the RV64
  prepared object call path consume existing prepared facts rather than
  deriving offsets from ABI index, assembly shape, object layout, filenames, or
  testcase shape.
- Added object-emission fail-closed coverage for missing argument destination
  size, missing transport destination offset, and mismatched transport
  destination offset, complementing the existing missing argument destination
  offset, mismatched transport destination size, and mismatched outgoing-area
  guards.
- Refreshed prepared-BIR probes for `20000808-1.c`, the `931004-*` family,
  `931031-1.c`, `950607-2.c`, and `pr69447.c` under the Step 4 artifact
  directory.
- `20000808-1.c`, odd `931004-*`, `931031-1.c`, `950607-2.c`, and
  `pr69447.c` now expose prepared destination facts where the row reaches the
  prepared call boundary; even `931004-*` rows remain earlier semantic
  local-memory rejections, not consumer guard blockers.

## Suggested Next

Proceed to Step 5: rerun the Step 1 diagnostic breadth set, compare first-owner
buckets before and after Steps 3 and 4, and decide whether idea 624 is
close-ready or needs a narrower follow-up for remaining non-consumer residuals.

## Watchouts

- Keep Step 5 focused on breadth/reclassification. Do not expand idea 624 into
  semantic local-memory repair for even `931004-*` rows or carrier alias
  publication residuals observed in `20000808-1.c`.
- The consumer guard shape is intentionally strict: argument destination offset
  must be zero for the byval stack-copy address consumer, argument destination
  size must equal the transport copy size, transport destination facts must
  match argument destination facts, and the call outgoing area must exactly
  cover the prepared destination size.

## Proof

Delegated proof command:
`cmake --build build --target backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_riscv_object_emission_test c4cll > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_riscv_object_emission|backend_dump_riscv64_byval_aggregate_fixed_call|backend_codegen_route_riscv64_byval_aggregate_fixed_call|backend_dump_riscv64_byval_preserved_pointer_args|backend_codegen_route_riscv64_byval_preserved_pointer_args|backend_codegen_route_riscv64_byval_formal_gpr_publication|backend_rv64_runtime_riscv64_byval_aggregate_fixed_call|backend_rv64_runtime_riscv64_byval_preserved_pointer_args|backend_rv64_runtime_riscv64_byval_formal_gpr_publication)$' >> test_after.log 2>&1`

Result: passed, 11/11 tests passed, preserved in `test_after.log`.

Diagnostic refresh command:
`./build/c4cll --dump-prepared-bir --target riscv64-linux-gnu <gcc torture row>`

Representative diagnostic output:
`build/agent_state/624_step4_consumer_guards/*.prepared.txt`
