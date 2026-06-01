Status: Active
Source Idea Path: ideas/open/84_aarch64_prepared_consumer_wrapper_contraction.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Cross-Owner Consistency And Closure Proof

# Current Packet

## Just Finished

Step 5 - Cross-Owner Consistency And Closure Proof cleanup removed the stale
prepared scalar ALU test references to the deleted instruction-record wrappers.
`backend_aarch64_prepared_scalar_alu_records_test.cpp` now calls the surviving
prepared record builders directly and constructs scalar instruction records with
`make_scalar_alu_instruction_record` or `make_scalar_unary_instruction_record`
before asserting the same instruction-level facts.

Contracted redundant wrappers:

- `make_prepared_scalar_alu_instruction_record` was removed from the ALU owner;
  `lower_scalar_instruction` now consumes `make_prepared_scalar_alu_record`
  directly and builds the existing `ScalarAluInstructionRecord` with
  `make_scalar_alu_instruction_record`.
- `make_prepared_scalar_unary_instruction_record` was removed from the ALU
  owner; this was a dead exported wrapper surface, and direct prepared-scalar
  tests now consume `make_prepared_scalar_unary_record` and build the existing
  scalar unary instruction record locally.
- `make_prepared_frame_slot_load_memory_instruction_record` was removed from
  the memory owner; direct prepared-memory tests use
  `make_prepared_load_memory_instruction_record(..., nullptr, ...)` for the
  same no-edge-publication path.

Retained helpers are intentionally not wrapper-cleanup candidates:

- Call-boundary helpers from the audit were retained because they own ABI
  register conversion, direct-vs-indirect call behavior, argument/result
  materialization, selected-source handling, and call-boundary diagnostics. This
  was a deliberate Step 2 classification, not a forgotten call cleanup.
- `make_prepared_load_memory_instruction_record`,
  `make_prepared_store_memory_instruction_record`, and
  `make_prepared_memory_operand_record` remain necessary prepared adapters that
  preserve addressing legality, storage-plan checks, edge-publication handling,
  diagnostics, and memory machine-record construction in the AArch64 memory
  owner.
- Store-source publication planning/lowering helpers are retained target-local
  emission paths because they own source materialization, scratch/addressing
  choices, and concrete store/writeback publication instructions.
- `make_prepared_scalar_alu_record`, `make_prepared_scalar_unary_record`, and
  scalar prepared operand/result helpers remain necessary prepared adapters;
  they keep prepared name/home/storage facts flowing into AArch64-local scalar
  records without reconstructing authority under a new name.
- Scalar compare/select publication helpers are retained target-local emission
  paths because they own compare/select materialization, condition spelling,
  scratch choices, stack publication, and emitted-scalar state updates.

Confirmed no remaining `make_prepared_scalar_alu_instruction_record` or
`make_prepared_scalar_unary_instruction_record` references in `src` or `tests`
outside this `todo.md` status note.

No source-idea closure is claimed here; this packet only prepares the execution
state and proof notes for the supervisor/plan-owner closure decision.

## Suggested Next

Supervisor/plan-owner closure decision for active idea 84. Do not infer source
idea closure from this executor packet alone.

## Watchouts

- The closure proof intentionally retains large call, store-source publication,
  and scalar compare/select helpers as target-local emission or necessary
  prepared adapters, not as remaining redundant wrappers.
- This packet did not touch implementation files, `plan.md`, source idea files,
  memory/call files, or transient `review/` artifacts.

## Proof

Supervisor-selected cleanup proof command:

```sh
bash -o pipefail -c '(cmake --build build --target backend_aarch64_prepared_scalar_alu_records_test c4c_backend c4cll -- -j2 && ctest --test-dir build -R "backend_aarch64_prepared_scalar_alu_records|backend_aarch64_scalar_alu_records|backend_aarch64_scalar_record_contract|backend_codegen_route_aarch64_alu_unpublished_load_local_after_call|backend_codegen_route_aarch64_alu_unpublished_load_local_call_boundary|backend_codegen_route_aarch64_scalar_fp_literal_add_publishes_fpr_result" --output-on-failure) 2>&1 | tee test_after.log'
```

Result: build passed; 6/6 selected AArch64 scalar ALU tests passed.
Proof log: `test_after.log`.

Also ran:

```sh
git diff --check
```

Result: passed.
