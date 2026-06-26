Status: Active
Source Idea Path: ideas/open/390_rv64_va_list_value_publication_copy_runtime_abort.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Implement Narrow `va_list` Value Publication/Copy

# Current Packet

## Just Finished

Step 4 implemented the narrow RV64 prepared-call payload-publication route for
`FrameSlotAddress` call arguments.

The object route now requires all of the following before it emits the
publication/copy:

- a prepared `FrameSlotAddress` missing-publication need for the argument
- a supported frame-slot address materialization for the argument object
- exactly one available `StoreLocalPublication` prepared store-source record
  targeting that argument object's frame slot and stack offset
- a named pointer source value with an authoritative prepared value home

When those facts line up, RV64 object emission copies the initialized source
value into the frame-slot-address argument object with an 8-byte stack store
before emitting `addi aN, sp, <object-offset>`. Missing, mismatched, duplicate,
or malformed publication facts still fail closed through the prepared-call
unsupported-instruction path.

Focused coverage now asserts the accepted publication sequence and rejects
absent, duplicate, and destination-mismatched publication facts, while
preserving the Step 3 address-materialization fail-closed variants.

## Suggested Next

Execute Step 5: rerun the `va-arg-13.c` representative comparison and record
whether the current runtime abort advances or exposes a later owned boundary.

## Watchouts

- Keep idea 389 `va_start` destination-address materialization closed.
- Keep idea 388 `va_end`, idea 386 ordinary frame-slot-address GPR call
  arguments, and idea 387 sret out of scope.
- Do not hard-code `va-arg-13.c`, `test`, `dummy`, registers, stack offsets, or the abort branch.
- Do not broaden this packet into a full `va_arg`, aggregate, or call ABI rewrite.
- Address materialization alone is not proof that the initialized `va_list`
  payload reached `dummy`; the implementation must require a source-to-object
  publication/copy fact and fail closed when the fact is absent or ambiguous.
- The Step 4 route intentionally consumes prepared store-source publication
  facts; do not weaken this to address-materialization-only acceptance.
- Step 5 should distinguish this route from any later `va_arg`, `va_end`, or
  aggregate-copy boundary if `va-arg-13.c` still fails.

## Proof

Command run:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed.

Log path: `test_after.log`.
