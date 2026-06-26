Status: Active
Source Idea Path: ideas/open/390_rv64_va_list_value_publication_copy_runtime_abort.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify The Publication/Copy Route

# Current Packet

## Just Finished

Step 2 classified the RV64 `va_list` value-publication/copy route for
`va-arg-13.c`.

Selected route:

- Copy the initialized `%lv.state.8` payload into each frame-slot-address call
  argument object at the prepared call boundary, before materializing the
  address register passed to `dummy`.
- Do not republish by changing `fragment_for_prepared_variadic_va_start`.
  Step 1 showed `va_start` already initializes the prepared destination slot
  at slot #13 / offset 72 through `dst_va_list_addr=s1`.
- Do not introduce a new alias model for this narrow slice. The prepared call
  plan already identifies the `%t7` / `%t14` objects and the frame-slot-address
  arguments; the missing fact is that their payload slots must be populated
  from the initialized `va_list` source before their addresses are passed.

Implementable guards:

- Apply only in the RV64 prepared object `fragment_for_prepared_call` path for
  a GPR destination argument whose source selection kind is
  `FrameSlotAddress`.
- Require the current frame-slot-address guards already enforced by
  `prepared_frame_slot_address_call_argument_offset`: static fixed frame,
  no frame pointer for fixed slots, GPR value bank, frame-slot source encoding,
  source value id and source slot id present and matching the selection,
  stack-slot source home kind, unique default-address-space frame-slot
  materialization for the same source slot/value, matching materialization
  offset, non-negative signed-12-bit stack offset inside the selected frame
  slot, and no TLS address materialization.
- Additionally require an explicit prepared publication/copy fact tying the
  frame-slot-address argument object back to the initialized `va_list` source:
  for `%t7`, destination value id 15 / frame slot #9 must copy from
  `%lv.state.8` value id 10 / `dst_va_list` slot #13; for `%t14`,
  destination value id 19 / frame slot #10 must copy from the same source.
- Require the copy size and alignment to match the RV64 `va_list` layout from
  the variadic entry plan (`size=8`, `align=8`, one
  `overflow_arg_area` field at offset 0 for the current case), and require
  source and destination offsets to be encodable by the existing RV64 stack
  load/store helpers.
- Emit the payload copy before the existing `addi aN, sp, <offset>` address
  materialization for the argument. Ambiguous, missing, duplicate, mismatched,
  non-stack-slot, non-default-address-space, dynamic-frame, oversized, or
  unencodable shapes must return `std::nullopt`.

Required prepared facts for implementation:

- The variadic entry helper operands must expose the initialized source
  `dst_va_list` home and value id for `%lv.state.8`.
- The call argument plan/selection must expose the by-value copy destination
  value id, frame slot id, address-materialization frame slot id, and byte
  offset for `%t7` / `%t14`.
- A store-source/publication fact must connect the destination object payload
  back to the initialized `%lv.state.8` value rather than relying on local
  address materialization alone.

Fail-closed owner:

- Keep rejection in the RV64 prepared call fragment. The existing owner is the
  `fragment_for_prepared_call` / `prepared_frame_slot_address_call_argument_offset`
  unsupported route, surfaced as
  `unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering`
  for malformed frame-slot-address shapes; stack-frame encodability remains the
  existing unsupported-stack-frame diagnostic.

Scope separation:

- This does not reopen idea 389 because `va_start` destination-address
  materialization remains unchanged and already stores the cursor into the
  prepared destination slot.
- This does not reopen idea 388 because `va_end` is not involved.
- This does not reopen idea 386 because ordinary frame-slot-address GPR
  arguments continue to use the existing address-materialization path; this
  packet only adds a guarded payload-population prerequisite when a prepared
  publication/copy fact says the object payload is missing.
- This does not reopen idea 387 because no sret, aggregate return, or return
  ABI path is touched.

## Suggested Next

Execute Step 3: add focused backend coverage for the guarded RV64 prepared-call
payload copy route and its fail-closed edges. The coverage should describe the
accepted `%lv.state.8` to `%t7` / `%t14` publication-copy facts and the
missing/ambiguous fact shapes that must still reject. Do not implement the
route in Step 3; implementation belongs to Step 4.

## Watchouts

- Keep idea 389 `va_start` destination-address materialization closed.
- Keep idea 388 `va_end`, idea 386 ordinary frame-slot-address GPR call
  arguments, and idea 387 sret out of scope.
- Do not hard-code `va-arg-13.c`, `test`, `dummy`, registers, stack offsets, or the abort branch.
- Do not broaden this packet into a full `va_arg`, aggregate, or call ABI rewrite.
- Address materialization alone is not proof that the initialized `va_list`
  payload reached `dummy`; the implementation must require a source-to-object
  publication/copy fact and fail closed when the fact is absent or ambiguous.

## Proof

Command run: not rerun for this packet.

Reason: Step 2 is todo-only route classification with no implementation or
test changes. The delegated proof command remains:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: unchanged from Step 1 evidence; `test_after.log` is not present in the
current workspace, so no new proof log was created.
