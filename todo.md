Status: Active
Source Idea Path: ideas/open/84_aarch64_prepared_consumer_wrapper_contraction.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Contract ALU And Scalar Control Wrappers

# Current Packet

## Just Finished

Step 4 - Contract ALU And Scalar Control Wrappers completed the redundant unary
prepared scalar instruction wrapper contraction. Removed
`make_prepared_scalar_unary_instruction_record` from `alu.cpp`, `alu.hpp`, and
the aggregate declaration in `instruction.hpp`.

`make_prepared_scalar_unary_record` and
`make_scalar_unary_instruction_record` remain in place as the prepared-record
and instruction-record builders. No live unary wrapper call site existed, so no
lowering fallback path needed a call-site fold.

## Suggested Next

For the next bounded packet, either continue with another Step 4 ALU/control
wrapper candidate after supervisor review, or move to Step 5 consistency proof
if the supervisor considers the ALU redundant-wrapper path sufficient.

## Watchouts

- Do not fold scalar compare/select publication helpers into this cleanup path;
  those are target-local emission helpers with scratch and fallback behavior.
- `lower_scalar_control_value_instruction` still has the previously noted
  apparent unreachable scalar ALU fallback branch after the first binary branch.
  That remains a separate ALU/control-flow packet.
- Confirmed no remaining `make_prepared_scalar_unary_instruction_record`
  references outside this `todo.md` handoff.
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
