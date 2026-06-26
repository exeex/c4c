Status: Active
Source Idea Path: ideas/open/408_prepared_va_start_destination_address_helper_operand_publication.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Classify The Missing Helper Operand Publication

# Current Packet

## Just Finished

Step 1 classified the missing `va_start` helper operand publication for
`va-arg-21.c`. The exact delegated proof completed successfully and confirmed
that the prepared variadic entry plan has enough RV64 variadic state to identify
the gap: the destination `va_list` address home is frame-slot backed, but the
current RV64 helper-operand authority only keeps register-backed destination
addresses.

Fresh prepared facts:

- `prepared.func @doit named_params=1 named_gp=1 named_fp=0 helpers=1`.
- `overflow_area required=yes base_slot=#31 base_stack_offset=232 align=8`.
- `va_list_layout required=yes size=8 align=8 fields=1` with
  `field kind=overflow_arg_area offset=0 size=8`.
- `helper_resources scratch_registers=3 scratch_stack=0 helpers=[va_start]`.
- `helper kind=va_start` is present, followed by incoming RV64 variadic GPR
  publications from `a1`..`a7` into overflow slot `#31` at stack offsets
  `232`..`280`.
- The only missing helper fact in this surface is
  `missing fact=helper_operand_homes.va_start.destination_va_list_address`.

Destination-address evidence:

- The two `llvm.va_start.p0` call sites are `bir.call void
  llvm.va_start.p0(ptr %t1)` and `bir.call void llvm.va_start.p0(ptr %t6)`.
- `%t1` has `storage %t1 frame_slot bank=gpr spill_slot=slot#17+stack136
  offset=136`, `home %t1 kind=stack_slot slot_id=17 offset=136`, and later
  `encoding=frame_slot bank=gpr ... stack_offset=136`.
- `%t6` has `storage %t6 frame_slot bank=gpr spill_slot=slot#18+stack144
  offset=144`, `home %t6 kind=stack_slot slot_id=18 offset=144`, and later
  `encoding=frame_slot bank=gpr ... stack_offset=144`.
- Prepared call-argument facts for the two `llvm.va_start.p0` calls publish
  complete GPR destinations to `a0`, but with `source_encoding=frame_slot`
  for source values `%t1` and `%t6`. This is a frame-slot-backed destination
  address, not a register-backed one.
- Address-materialization facts also exist for related frame-slot va-list
  objects, including `%lv.ap_array.0`, `%t17`, `%t20`, `%t23`, and `%t32`.

Producer path:

- `src/backend/prealloc/variadic_entry_plans.cpp` is the producer area.
- `materialize_va_start_destination_home()` creates the destination `va_list`
  storage home as a synthetic stack slot using the active `va_list_layout`.
- `populate_rv64_variadic_entry_va_start_operand_home_authority()` walks
  `llvm.va_start.p0` calls, sets `homes.destination_va_list_address` from
  `prepared_home_for_named_value(..., call->args[0])`, then derives
  `homes.destination_va_list` with
  `materialize_va_start_destination_home(..., "rv64", *homes.destination_va_list_address)`.
- The same function currently clears
  `homes.destination_va_list_address` unless its kind is
  `PreparedValueHomeKind::Register`. For this testcase the call operand homes
  are stack slots, so `destination_va_list` can be materialized but
  `destination_va_list_address` is deliberately nulled.
- `require_variadic_helper_operand_home()` then appends the missing fact, and
  `has_complete_prepared_variadic_va_start_operand_homes()` prevents a
  `helper_operand kind=va_start ...` line from being published.

Classification:

- This is a producer-side helper operand publication gap, not an RV64 object
  emission gap.
- Step 2 should publish/support frame-slot-backed `destination_va_list_address`
  homes for RV64 `va_start` when the call operand is a prepared frame-slot
  value with complete slot/offset/bank facts. Keep the existing register-backed
  path working.
- Do not implement target-side inference. The producer already has the call
  operand home; the incomplete rule is the register-only filter in the helper
  operand authority.

## Suggested Next

Implement Step 2 in `src/backend/prealloc/variadic_entry_plans.cpp`: update
the RV64 `va_start` helper operand authority so a stack-slot/frame-slot call
operand home remains published as `destination_va_list_address` when it carries
complete prepared slot facts. Add or update a focused prepared-printer/backend
test covering the live frame-slot-backed address shape from `va-arg-21.c`.

## Watchouts

- Do not infer the destination `va_list` address in RV64 object emission.
- Keep destination `va_list` storage distinct from destination `va_list`
  address metadata.
- Preserve precise unsupported diagnostics for unsupported helper operand
  shapes.
- Do not use filename-specific handling, expectation rewrites, or allowlist
  filtering as progress.
- The existing tests include a register-backed RV64 `dst_va_list_addr` contract;
  Step 2 must add the frame-slot-backed variant rather than weakening that
  check.
- The helper operand line is absent in the fresh dump because the homes are not
  complete; compare against the missing fact line rather than assuming the
  helper is absent from the variadic helper resource plan.

## Proof

Exact command run:

```sh
cmake --build --preset default && mkdir -p build/agent_state/408_step1_va_start_address && build/c4cll --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/va-arg-21.c > build/agent_state/408_step1_va_start_address/va-arg-21.prepared.txt 2> build/agent_state/408_step1_va_start_address/va-arg-21.prepared.err && build/c4cll --target riscv64-linux-gnu tests/c/external/gcc_torture/src/va-arg-21.c > build/agent_state/408_step1_va_start_address/va-arg-21.out 2> build/agent_state/408_step1_va_start_address/va-arg-21.err && rg -n 'helper kind=va_start|helper_operand kind=va_start|missing fact=helper_operand_homes.va_start.destination_va_list_address|dst_va_list|dst_va_list_addr|va_list_layout|overflow_area|incoming_variadic|llvm\.va_start|storage %|home %|frame_slot|stack_slot|encoding=|bank=|reg=|va_start|va_list|unsupported|error|fatal' build/agent_state/408_step1_va_start_address/va-arg-21.prepared.txt build/agent_state/408_step1_va_start_address/va-arg-21.prepared.err build/agent_state/408_step1_va_start_address/va-arg-21.out build/agent_state/408_step1_va_start_address/va-arg-21.err && rg -n 'helper_operand_homes|destination_va_list_address|destination_va_list|VaStart|va_start|va_list_layout|find_prepared_variadic_entry_helper_operand_homes|has_complete_prepared_variadic_va_start_operand_homes' src/backend tests/backend/bir | sed -n '1,240p' && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result:

- Command completed successfully.
- `cmake --build --preset default`: no work to do.
- Fresh `va-arg-21.c` prepared dump and object-route command both completed.
- The expected missing fact remains in the prepared dump:
  `helper_operand_homes.va_start.destination_va_list_address`.
- Source search located the producer path in
  `src/backend/prealloc/variadic_entry_plans.cpp`.
- `test_after.log`: `100% tests passed, 0 tests failed out of 326`.
