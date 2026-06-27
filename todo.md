Status: Active
Source Idea Path: ideas/open/408_prepared_va_start_destination_address_helper_operand_publication.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Prove Representative And Residual Ownership

# Current Packet

## Just Finished

Step 3 completed the final 408 representative proof/routing check for
`va-arg-21.c`. The old producer-side helper operand gap remains fixed in a
fresh prepared dump: both frame-slot-backed RV64 `llvm.va_start.p0` call
operands publish `destination_va_list_address` homes, and the old missing fact
is absent.

- `helper kind=va_start`.
- `helper_operand kind=va_start block=0 inst=7
  dst_va_list=%t1:stack_slot:slot=#32:offset=288
  dst_va_list_addr=%t1:stack_slot:slot=#17:offset=136`.
- `helper_operand kind=va_start block=0 inst=26
  dst_va_list=%t6:stack_slot:slot=#33:offset=296
  dst_va_list_addr=%t6:stack_slot:slot=#18:offset=144`.
- The old `missing fact=helper_operand_homes.va_start.destination_va_list_address`
  diagnostic is absent from the fresh prepared dump and stderr; the explicit
  absence check passed.

`build/c4cll --target riscv64-linux-gnu
tests/c/external/gcc_torture/src/va-arg-21.c` completed successfully, so no
fresh residual appeared in this representative. This routes as closure-ready
for the 408 helper-operand publication idea, subject to plan-owner lifecycle
review. No implementation files were changed in Step 3.

## Suggested Next

Ask the plan owner to review/close active idea 408, or to retire the runbook if
they require broader lifecycle evidence.

## Watchouts

- Do not infer the destination `va_list` address in RV64 object emission.
- Keep destination `va_list` storage distinct from destination `va_list`
  address metadata.
- Preserve precise unsupported diagnostics for unsupported helper operand
  shapes.
- Do not use filename-specific handling, expectation rewrites, or allowlist
  filtering as progress.
- `backend_prepare_frame_stack_call_contract` now contains the committed
  frame-slot-backed guard; `backend_prepared_printer` still covers the
  register-backed printer path.
- Step 3 found no separate residual owner for `va-arg-21.c`.

## Proof

Exact command run:

```sh
cmake --build --preset default && mkdir -p build/agent_state/408_step3_va_start_proof && build/c4cll --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/va-arg-21.c > build/agent_state/408_step3_va_start_proof/va-arg-21.prepared.txt 2> build/agent_state/408_step3_va_start_proof/va-arg-21.prepared.err && build/c4cll --target riscv64-linux-gnu tests/c/external/gcc_torture/src/va-arg-21.c > build/agent_state/408_step3_va_start_proof/va-arg-21.out 2> build/agent_state/408_step3_va_start_proof/va-arg-21.err && rg -n 'helper kind=va_start|helper_operand kind=va_start|dst_va_list=|dst_va_list_addr=|missing fact=helper_operand_homes.va_start.destination_va_list_address|unsupported|error|fatal' build/agent_state/408_step3_va_start_proof/va-arg-21.prepared.txt build/agent_state/408_step3_va_start_proof/va-arg-21.prepared.err build/agent_state/408_step3_va_start_proof/va-arg-21.out build/agent_state/408_step3_va_start_proof/va-arg-21.err && if rg -n 'missing fact=helper_operand_homes.va_start.destination_va_list_address' build/agent_state/408_step3_va_start_proof/va-arg-21.prepared.txt build/agent_state/408_step3_va_start_proof/va-arg-21.prepared.err; then exit 1; fi && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result:

- Command completed successfully.
- `cmake --build --preset default`: no work to do.
- Fresh prepared dump published both frame-slot-backed `dst_va_list_addr`
  helper operand homes for `va-arg-21.c` at block/instruction pairs `0/7`
  and `0/26`.
- The absence check for
  `missing fact=helper_operand_homes.va_start.destination_va_list_address`
  passed.
- `build/c4cll --target riscv64-linux-gnu tests/c/external/gcc_torture/src/va-arg-21.c`
  completed successfully.
- `test_after.log`: backend subset passed, `100% tests passed, 0 tests failed
  out of 326`.
