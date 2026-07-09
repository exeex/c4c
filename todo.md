Status: Active
Source Idea Path: ideas/open/624_prepared_outgoing_stack_argument_destination_offsets.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Publish prepared outgoing destination facts

# Current Packet

## Just Finished

Completed Step 3 regression repair: preserved prepared outgoing destination
facts for complete RV64 byval/memory-class outgoing stack-copy call arguments
while restoring the existing RV64 byval dump, codegen-route, object, and
runtime behavior.

Artifact:
`build/agent_state/624_step3_prepared_destination_facts/after_rows.txt`

Result:

- Kept the RV64 upstream stack-destination authority predicate in
  `regalloc_detail::call_arg_destination_stack_offset_bytes(...)` so complete
  byval/memory-class stack destinations with explicit size/alignment are
  accepted while missing, zero-sized, sret, or incomplete ABI authority stays
  rejected.
- Kept `plan_call_argument_destination(...)` publishing destination stack
  offsets/sizes from the upstream helper when a stack ABI binding exists but
  does not already carry the offset.
- Repaired the RV64 text and object byval stack-copy consumers to accept
  prepared destination facts only when the argument destination offset is zero,
  destination size equals the aggregate transport copy size, transport
  destination facts match the argument facts, and `outgoing_stack_argument_area`
  covers that destination size.
- Added RV64 object-emission guard coverage for missing destination offset,
  mismatched transport destination size, and mismatched outgoing area so
  incomplete/inconsistent prepared facts remain fail-closed.
- Kept prepared dump compatibility with existing RV64 byval dump snippets while
  still printing `dest_stack_offset`, `dest_stack_size`,
  `transport_dest_stack_offset`, and `transport_dest_stack_size`.
- Representative prepared dumps still show explicit destination facts for
  `src/20000808-1.c`, odd `src/931004-*`, `src/931031-1.c`, and
  `src/950607-2.c`; `src/pr69447.c` remains the scalar stack guard with
  `dest_stack_offset=0`, `dest_stack_size=8`, and
  `outgoing_stack_argument_area=8`.

## Suggested Next

Proceed to Step 4: wire RV64 consumers to require the prepared destination
offset and size facts without deriving them from ABI index, final assembly
shape, source filenames, or testcase shape. The existing RV64 byval stack-copy
text/object consumers now provide a complete-facts guard pattern for that work.

## Watchouts

- The destination offsets come from
  `regalloc_detail::call_arg_destination_stack_offset_bytes(...)`; downstream
  RV64 code should consume or validate these facts, not rederive them.
- Complete RV64 byval/memory stack destinations require byval pointer ABI,
  memory primary class, `passed_on_stack`, nonzero size, and nonzero alignment.
- Missing destination offset, destination size, outgoing area, or incomplete
  aggregate transport coverage should remain fail-closed for consumers.
- The RV64 prepared dump printer keeps old aggregate-address summary/detailed
  substring ordering for existing dump tests; the detailed call-plan rows still
  include the new destination facts later in the line.

## Proof

Delegated proof command:
`cmake --build build --target backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_riscv_object_emission_test c4cll > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_riscv_object_emission|backend_dump_riscv64_byval_aggregate_fixed_call|backend_codegen_route_riscv64_byval_aggregate_fixed_call|backend_dump_riscv64_byval_preserved_pointer_args|backend_codegen_route_riscv64_byval_preserved_pointer_args|backend_codegen_route_riscv64_byval_formal_gpr_publication|backend_rv64_runtime_riscv64_byval_aggregate_fixed_call|backend_rv64_runtime_riscv64_byval_preserved_pointer_args|backend_rv64_runtime_riscv64_byval_formal_gpr_publication)$' >> test_after.log 2>&1`

Result: passed, 11/11 tests passed, preserved in `test_after.log`.

Diagnostic refresh command:
`./build/c4cll --dump-prepared-bir --target riscv64-linux-gnu <representative case>`

Representative diagnostic output:
`build/agent_state/624_step3_prepared_destination_facts/after_rows.txt`
