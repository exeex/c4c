Status: Active
Source Idea Path: ideas/open/661_rv64_prepared_destination_publication.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Refresh Prepared Destination Evidence

# Current Packet

## Just Finished

Step 1: Refresh Prepared Destination Evidence ran the delegated focused dump
subset for the four active RV64 prepared-destination rows and preserved
`test_after.log`. Build succeeded (`ninja: no work to do`); CTest completed
0/4 passing because each row still reports `BACKEND_DUMP_SNIPPET_MISSING`.

Focused evidence:

- `backend_dump_riscv64_stack_passed_parameter_home_publication`: first
  failing snippet is
  `abi_binding destination_kind=call_argument_abi destination_storage=stack_slot abi_index=8 stack_offset=0`.
  Current prepared output instead publishes ABI stack bindings for indices 9,
  11, and 12, while index 8 is published as register `a7` and index 10 as
  register `fa1`. Callee parameter homes for stack-passed parameters are present
  but with current IDs/slots (`%p.fdB` value_id 10 slot#10 offset 40, `%p.C`
  value_id 12 slot#12 offset 48, `%p.fdC` value_id 13 slot#11 offset 44).
  Recommendation: split to parameter-home publication plus dump-contract review;
  do not route through ideas 647/655 fan-in authority from this evidence.
- `backend_dump_riscv64_scalar_compare_frame_slot_destination`: first failing
  snippet is `home %t2 value_id=2 kind=stack_slot slot_id=5 offset=24`.
  Current prepared output has the frame-slot destination fact as
  `home %t2 value_id=3 kind=stack_slot slot_id=5 offset=24`, plus
  `storage %t2 frame_slot ... offset=24` and the expected store-source
  publication for `bir.store_local %lv.comparison`. Recommendation: separate
  dump-contract/value-id review owner before any prepared destination repair.
- `backend_dump_riscv64_prepared_fused_compare_call_result_predicate`: first
  failing snippet is
  `result value_bank=gpr source_storage=register destination_storage=register destination_value_id=0`.
  Current prepared output publishes the call-result destination as
  `destination_value_id=1`, with an `after_call` call-result ABI move and
  branch condition consuming `%t1`. Recommendation: separate dump-contract
  review unless a later route/runtime proof shows RV64 consumer lowering is
  still wrong.
- `backend_dump_riscv64_function_pointer_return_chain`: first failing snippet
  is `home @sub value_id=5 kind=register reg=t0`. Current prepared output has
  the function pointer constant as `home @sub value_id=6 kind=register reg=t0`,
  records the indirect callee `%t1` as `indirect_value_id=8`, and publishes the
  indirect call result as destination value_id 9. Recommendation: separate
  return-chain dump-contract/value-id review owner, not prepared destination
  publication.

## Suggested Next

Supervisor should route Step 2 as a boundary-selection/lifecycle decision
rather than an implementation packet. The focused Step 1 evidence does not
identify a prepared destination publication repair boundary; it primarily shows
stale dump snippets/value IDs and one parameter-home/dump-contract split.

## Watchouts

- Keep stack-destination fan-in authority from ideas 647 and 655 out of this
  route unless evidence requires supervisor lifecycle routing.
- The scalar compare, fused compare call-result, and function-pointer
  return-chain rows all show the requested prepared facts with current value
  IDs, so changing lowering before dump-contract review risks overfitting stale
  snippets.
- The stack-passed parameter-home row's first failure is caller ABI stack
  binding placement, while callee stack-parameter homes are present under
  current IDs. Treat it as parameter-home/dump-contract split evidence, not as
  stack-destination fan-in authority.
- Do not change expectations, unsupported markers, allowlists, runtime policy,
  timeout policy, baseline accounting, or test classification.
- Do not absorb pointer-local, byval, object-data static storage, AArch64,
  prepared CLI, RISC-V object emission, callee-saved GPR, packed-member, or
  LLVM torture work into this packet.

## Proof

Exact proof command run:

```sh
cmake --build --preset default && (ctest --test-dir build -j --output-on-failure -R 'backend_dump_riscv64_(stack_passed_parameter_home_publication|scalar_compare_frame_slot_destination|prepared_fused_compare_call_result_predicate|function_pointer_return_chain)' > test_after.log; test -s test_after.log)
```

Result: build succeeded; focused CTest subset produced `test_after.log` and
completed 0/4 passing with all four rows failing by
`BACKEND_DUMP_SNIPPET_MISSING`.
