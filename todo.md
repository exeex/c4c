Status: Active
Source Idea Path: ideas/open/366_rv64_va_start_destination_va_list_gpr_home.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Destination Address Producer Boundary

# Current Packet

## Just Finished

Completed `plan.md` Step 1 audit for
`ideas/open/366_rv64_va_start_destination_va_list_gpr_home.md`.

Inspection commands/results:

- `sed -n '1,240p' plan.md`, `sed -n '1,260p' todo.md`, and
  `sed -n '1,260p' ideas/open/366_rv64_va_start_destination_va_list_gpr_home.md`
  confirmed this packet is the active Step 1 audit and that the goal is an
  explicit prepared destination-address GPR-home contract, not object-emission
  guessing.
- `sed -n '1,280p' ideas/closed/365_rv64_va_start_overflow_base_state_production.md`
  confirmed the prior milestone already produced RV64 overflow-area initial
  base state; the residual is the later destination-address boundary.
- `sed -n '1,240p' build/rv64_gcc_c_torture_backend/src_920908-1.c/case.log`
  found the representative failure:
  `unsupported_variadic_helper_lowering: RV64 va_start helper requires destination va_list address in a prepared GPR home`.
- `rg -n "destination_va_list|va_start helper requires destination|materialize_va_start_destination_home|helper_operand_homes" src/backend/prealloc src/backend/mir/riscv/codegen tests/backend -S`
  located the producer, printer/test coverage, and RV64 consumer.
- `sed -n '220,340p' src/backend/prealloc/variadic_entry_plans.cpp` showed
  `materialize_va_start_destination_home()` creates or finds a
  `rv64_variadic_va_start_destination.<value_name>` frame object and returns
  `PreparedValueHomeKind::StackSlot` for `destination_va_list_address`.
- `sed -n '598,615p' src/backend/prealloc/variadic_entry_plans.cpp` and the
  RV64 helper loop around `populate_rv64_variadic_entry_helper_operand_home_authority()`
  showed `destination_va_list` is copied from prepared value locations for
  `call->args[0]`, then `destination_va_list_address` is derived by
  `materialize_va_start_destination_home()` and recorded as complete if present.
- `sed -n '260,360p' src/backend/mir/riscv/codegen/object_emission.cpp` and
  `sed -n '730,775p' src/backend/mir/riscv/codegen/object_emission.cpp` showed
  the RV64 consumer requires `homes.destination_va_list_address` to be
  `PreparedValueHomeKind::Register` with a valid GPR spelling before emitting
  the helper stores.
- `nl -ba tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp | sed -n '8150,8265p'`
  showed focused prepared-contract coverage already requires RV64
  `destination_va_list_address` not to be a missing fact, but does not require
  it to be a GPR home.
- `nl -ba tests/backend/mir/backend_riscv_object_emission_test.cpp | sed -n '400,510p'`
  and `sed -n '3120,3168p'` showed the RV64 object-emission unit fixture can
  lower `va_start` only when the manually prepared address home is a register
  (`a1`); that fixture proves the consumer contract, not the semantic producer.
- `nl -ba tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp | sed -n '4560,4725p'`
  found an adjacent explicit local-frame address materialization contract
  (`PreparedAddressMaterializationKind::FrameSlot` plus call-argument source
  selection) that is likely the right producer-side pattern to reuse or mirror.

Finding: the missing state is a producer/publication gap, not an absent helper
fact and not a consumer contract mismatch. The prepared RV64 helper homes
publish `destination_va_list` and `destination_va_list_address`, and the missing
fact list no longer retains
`helper_operand_homes.va_start.destination_va_list_address`. However, the
published address home is currently the stack slot allocated for the synthetic
destination `va_list` object, not a GPR containing that slot address. RV64
object emission correctly refuses to synthesize a base register because helper
stores need a real prepared GPR base.

First implementation boundary: keep the RV64 object consumer check in
`src/backend/mir/riscv/codegen/object_emission.cpp` intact, and change the
producer side around `materialize_va_start_destination_home()` /
`populate_rv64_variadic_entry_helper_operand_home_authority()` so supportable
`va_start` destinations publish a prepared address value home in a GPR (or an
explicit address-materialization fact that is tied to a GPR home) while the
synthetic `va_list` storage object remains a stack/frame slot.

## Suggested Next

Step 2 should define and test the prepared destination-address contract before
implementation: add focused prepared-contract/printer assertions that RV64
`va_start` helper operand homes distinguish the destination `va_list` storage
home from its address home, and require the address home to be a prepared GPR
home for supported lowering. Then implement the narrow producer path in
`src/backend/prealloc/variadic_entry_plans.cpp`; do not weaken the RV64 object
consumer.

Recommended narrow proof command for the Step 2 implementation packet:

```sh
cmake --build build --target backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_riscv_object_emission_test && ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_riscv_object_emission)$' --output-on-failure
```

## Watchouts

- Treat `src/920908-1.c` as a representative signal, not an implementation key.
- Do not reopen idea-365 overflow-area base-state production.
- Do not broaden into general parameter-home expansion, external variadic call
  lowering, scalar `va_arg`, `va_copy`, or aggregate `va_arg`.
- Progress must publish or prove an explicit prepared destination-address
  contract; a diagnostic rename, expectation rewrite, or object-emission guess
  is not enough.
- Current focused prepared-contract tests assert the RV64
  `destination_va_list_address` fact exists but not that it is a register; Step
  2 should add that distinction so the producer gap is caught before object
  emission.
- `materialize_va_start_destination_home()` currently conflates "the
  destination object storage exists" with "the destination object's address is
  available in a GPR"; preserve the storage slot, but publish a separate
  supportable address-home contract.
- If a supportable GPR address cannot be produced for a path, keep the current
  precise RV64 diagnostic instead of synthesizing from frame layout in object
  emission.

## Proof

No build required; this audit changed only `todo.md`.

Proof command:

```sh
git diff --check -- todo.md
```

Result: passed.
