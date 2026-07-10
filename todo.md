Status: Active
Source Idea Path: ideas/open/654_direct_global_stack_backed_pointer_branch_boundary.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Consume Direct-Global Branch Authority In RV64

# Current Packet

## Just Finished

Step 3 of `plan.md`: implemented RV64 consumption of explicit DirectGlobal
prepared address materialization for a selected stack-backed pointer branch
operand.

Changed files:
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `todo.md`

Implementation evidence:
- Added `append_rv64_direct_global_pointer_branch_source_to_register` on the
  RV64 pointer branch operand path.
- The consumer requires a named `ptr` operand with a stack-slot home, a unique
  same-value pre-terminator `PreparedAddressMaterializationKind::DirectGlobal`,
  `GlobalAddressMaterializationPolicy::Direct`, default address space, no TLS
  attributes, zero byte offset, and a valid prepared symbol name.
- Malformed same-value materialization candidates fail closed instead of
  falling through to generic stack-slot movement.
- Existing stack-carried and frame-slot materialized pointer-source paths remain
  ahead of the DirectGlobal path, so this does not reopen the idea-645 fused
  branch family or `%t6`/`%t23` stack-carried publication routes.

Focused coverage:
- Added a direct RHS stack-backed pointer branch fixture that publishes explicit
  DirectGlobal address materialization and verifies RV64 emits the PC-relative
  symbol address into the RHS branch register, with no RHS stack reload.
- Added fail-closed mutations for malformed same-value DirectGlobal authority:
  non-direct policy, non-DirectGlobal materialization kind, and ambiguous
  duplicate candidates.

## Suggested Next

Supervisor should decide whether to accept this Step 3 code slice despite the
pre-existing broad backend proof failures, or first route a separate regression
baseline cleanup packet. If accepted, the next packet should run Step 4
representative integration and classify the current `src/20000314-3.c` object
route owner after the same-module call ABI gate.

## Watchouts

- This slice intentionally does not treat absent DirectGlobal materialization
  as an error for ordinary stack-backed pointer branches; it only consumes or
  rejects explicit same-value DirectGlobal authority.
- The delegated backend subset currently fails broadly before this slice can be
  accepted by proof alone. The new focused fixture did not emit any failure
  message in the `backend_riscv_object_emission_test` output, but that test
  binary still returns failing status because of other existing checks.
- The full `src/20000314-3.c` object route was not used as Step 3 proof because
  Step 1 found it now stops first at `unsupported_call_abi` in `attr_eq`.

## Proof

Focused build:
- `cmake --build --preset default --target backend_riscv_object_emission_test`
  succeeded.

Delegated proof:
- Command: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
- Log: `test_after.log`
- Result: failed. Build completed, then CTest reported 32 failures out of 365
  backend tests. The failing set includes existing broad RV64 dump/runtime
  cases plus `backend_riscv_object_emission`, `backend_aarch64_instruction_dispatch`,
  `backend_prepare_liveness`, `backend_prepare_frame_stack_call_contract`,
  `backend_prepared_printer`, `backend_prealloc_inline_asm`, and three backend
  CLI prepared-BIR checks.

Supervisor regression comparison:
- Compared canonical `test_before.log` and `test_after.log` for the same
  backend subset. Both logs report 333 passed, 32 failed, 365 total; no new
  failing tests and no newly slow tests.
- Strict monotonic guard failed only because the passed count did not increase.
  The non-decreasing guard passed with `--allow-non-decreasing-passed`.
