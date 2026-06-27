Status: Active
Source Idea Path: ideas/open/408_prepared_va_start_destination_address_helper_operand_publication.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Publish The Supported Destination Address Fact

# Current Packet

## Just Finished

Step 2 implemented the producer-side RV64 `va_start` helper operand repair in
`src/backend/prealloc/variadic_entry_plans.cpp`. The RV64 helper operand
authority now keeps `destination_va_list_address` when the prepared home is
either a named register home or a complete stack-slot/frame-slot home with
slot id, offset, size, and alignment. Unsupported or incomplete homes still
fail closed by clearing the address fact.

Fresh `va-arg-21.c` prepared facts now publish the frame-slot-backed helper
operand addresses:

- `helper kind=va_start`.
- `helper_operand kind=va_start block=0 inst=7
  dst_va_list=%t1:stack_slot:slot=#32:offset=288
  dst_va_list_addr=%t1:stack_slot:slot=#17:offset=136`.
- `helper_operand kind=va_start block=0 inst=26
  dst_va_list=%t6:stack_slot:slot=#33:offset=296
  dst_va_list_addr=%t6:stack_slot:slot=#18:offset=144`.
- `%t1` and `%t6` remain distinct from the materialized destination
  `va_list` storage and have `encoding=frame_slot bank=gpr` storage facts.
- The old `missing fact=helper_operand_homes.va_start.destination_va_list_address`
  diagnostic is absent from the fresh prepared dump and stderr.

No RV64 object-emission inference was added. The object-route compile for
`va-arg-21.c` completed successfully after the producer fact was published.
`tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp` now
adds focused RV64 coverage for both helper operand address paths: the existing
register-backed `ap` `va_start` path and a second `va_start` whose pointer is
kept live across an intervening call so its prepared address home is a complete
stack-slot/frame-slot home. The test asserts the frame-slot-backed
`destination_va_list_address` stays published, includes slot id, offset, size,
and alignment, and remains distinct from the materialized destination
`va_list` storage slot. Existing focused prepared/backend tests stayed green,
including the register-backed printer coverage.

## Suggested Next

Run Step 3 as a proof/routing packet for the 408 representative and decide
whether the idea is closable or whether a distinct residual remains outside
the producer helper-operand publication surface.

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

## Proof

Exact command run:

```sh
cmake --build --preset default && mkdir -p build/agent_state/408_step2_va_start_address && build/c4cll --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/va-arg-21.c > build/agent_state/408_step2_va_start_address/va-arg-21.prepared.txt 2> build/agent_state/408_step2_va_start_address/va-arg-21.prepared.err && build/c4cll --target riscv64-linux-gnu tests/c/external/gcc_torture/src/va-arg-21.c > build/agent_state/408_step2_va_start_address/va-arg-21.out 2> build/agent_state/408_step2_va_start_address/va-arg-21.err && rg -n 'helper kind=va_start|helper_operand kind=va_start|dst_va_list=|dst_va_list_addr=|missing fact=helper_operand_homes.va_start.destination_va_list_address|storage %t[16]|home %t[16]|encoding=frame_slot bank=gpr|llvm\.va_start|unsupported|error|fatal' build/agent_state/408_step2_va_start_address/va-arg-21.prepared.txt build/agent_state/408_step2_va_start_address/va-arg-21.prepared.err build/agent_state/408_step2_va_start_address/va-arg-21.out build/agent_state/408_step2_va_start_address/va-arg-21.err && if rg -n 'missing fact=helper_operand_homes.va_start.destination_va_list_address' build/agent_state/408_step2_va_start_address/va-arg-21.prepared.txt build/agent_state/408_step2_va_start_address/va-arg-21.prepared.err; then exit 1; fi && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer)$' && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result:

- Command completed successfully.
- `cmake --build --preset default`: completed successfully.
- Fresh prepared dump published both frame-slot-backed `dst_va_list_addr`
  helper operand homes for `va-arg-21.c`.
- `backend_prepare_frame_stack_call_contract` includes the new synthetic RV64
  frame-slot-backed `va_start` helper operand guard.
- The absence check for
  `missing fact=helper_operand_homes.va_start.destination_va_list_address`
  passed.
- `build/c4cll --target riscv64-linux-gnu tests/c/external/gcc_torture/src/va-arg-21.c`
  completed successfully.
- Focused tests `backend_prepare_frame_stack_call_contract` and
  `backend_prepared_printer`: `100% tests passed, 0 tests failed out of 2`.
- `test_after.log`: backend subset passed, `100% tests passed, 0 tests failed
  out of 326`.
