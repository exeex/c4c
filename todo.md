Status: Active
Source Idea Path: ideas/open/84_aarch64_prepared_consumer_wrapper_contraction.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Contract ALU And Scalar Control Wrappers

# Current Packet

## Just Finished

Step 4 - Contract ALU And Scalar Control Wrappers completed the first bounded
ALU wrapper contraction from the Step 1 audit. Removed
`make_prepared_scalar_alu_instruction_record` from `alu.cpp`, `alu.hpp`, and
the stale aggregate declaration in `instruction.hpp`, then changed
`lower_scalar_instruction` to consume `make_prepared_scalar_alu_record`
directly before constructing the existing `ScalarInstructionRecord` with
`make_scalar_alu_instruction_record(*record)`.

The prepared-record nullopt path is preserved: failed prepared scalar ALU
record construction still leaves `scalar_record` unset so the existing
fallback lowering paths can run. The scalar unary prepared instruction wrapper
was intentionally left unchanged.

## Suggested Next

For the next bounded ALU packet, contract
`make_prepared_scalar_unary_instruction_record` in the same style if the
supervisor wants to continue the small redundant-wrapper path. Keep ownership
to `alu.cpp`, `alu.hpp`, `todo.md`, and `test_after.log`, and use the same ALU
record proof plus any unary-specific test name the supervisor selects.

## Watchouts

- Do not fold scalar compare/select publication helpers into this cleanup path;
  those are target-local emission helpers with scratch and fallback behavior.
- `lower_scalar_control_value_instruction` still has the previously noted
  apparent unreachable scalar ALU fallback branch after the first binary branch.
  That remains a separate ALU/control-flow packet.
- This packet did not touch call, memory, test, plan, source idea, or transient
  `review/` files.

## Proof

Supervisor-selected proof command ran successfully and wrote `test_after.log`:

```sh
bash -o pipefail -c '(cmake --build build --target c4c_backend c4cll -- -j2 && ctest --test-dir build -R "backend_aarch64_prepared_scalar_alu_records|backend_aarch64_scalar_alu_records|backend_aarch64_scalar_record_contract|backend_codegen_route_aarch64_alu_unpublished_load_local_after_call|backend_codegen_route_aarch64_alu_unpublished_load_local_call_boundary|backend_codegen_route_aarch64_scalar_fp_literal_add_publishes_fpr_result" --output-on-failure) 2>&1 | tee test_after.log'
```

Result: build passed; 6/6 selected tests passed.

Also ran:

```sh
git diff --check
```

Result: passed.
