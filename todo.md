Status: Active
Source Idea Path: ideas/open/613_abi_call_result_stack_frame_lowering.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement First Ordinary ABI Consumer

# Current Packet

## Just Finished

Completed Step 2, "Implement First Ordinary ABI Consumer", for the
pointer-base-plus-offset local-frame address materialization variant inside the
basic same-module GPR/void call/result group.

The RV64 object call-argument consumer now accepts explicit
`LocalFrameAddressMaterialization` facts whose selected source home is
`PointerBasePlusOffset` when the prepared materialization supplies a concrete
frame-slot byte offset and that offset matches the prepared pointer byte delta
from the source slot base. This removes the prior scalar-slot containment
requirement for that pointer-offset materialization path while keeping the
existing bounds, 12-bit immediate, source value, source stack offset, frame
plan, and materialization-record checks fail closed.

Representative outcomes from focused probes:

- `src/20021219-1.c` moved past `unsupported_call_abi`; it now stops
  downstream at
  `prepared_consumer_category=malformed_prepared_join_transfer_carrier`.
- `src/pr77767.c` still compiles through `--codegen obj`.
- `src/20020529-1.c` still stops at `unsupported_call_abi` because the first
  same-module call carries stack-slot preservation whose source endpoint lacks
  a concrete source register name for `%p.b`; consuming that would require
  prepared preservation-source authority, not target-local reconstruction.
- The `931004-*` and `931031-1.c` rows still stop at `unsupported_call_abi`;
  they involve aggregate-address/stack ABI argument transport rather than the
  scalar GPR pointer-offset path handled here.

Focused object-emission tests were added for the pointer-base-plus-offset call
argument path and for fail-closed missing/mismatched pointer-delta authority.

## Suggested Next

Continue Step 2 only if the supervisor wants another in-scope ordinary ABI
consumer packet. The next coherent packet is stack-slot preservation source
authority classification for `src/20020529-1.c`: either consume explicit
register-to-stack / stack-to-register preservation endpoints if prepared facts
publish source registers, or split the missing preservation-source publication
gap if they do not.

## Watchouts

- `src/20021219-1.c` is no longer a call ABI row; its next owner is prepared
  join-transfer carrier materialization, not ABI call/result lowering.
- Do not infer stack preservation source registers from ABI parameter position
  or final assembly shape; require explicit prepared endpoints.
- Keep `931004-*` / `931031-1.c` aggregate stack argument transport, idea-624
  outgoing-stack destination offsets, memory-return/sret, FPR lanes,
  frame-slot publication, stack-frame, return stack-to-register, local/global
  producer, variadic, library, and runtime rows outside this packet unless the
  supervisor changes the boundary.

## Proof

Ran exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: passed, `346/346` backend tests; `test_after.log` is the preserved
proof log.
