Status: Active
Source Idea Path: ideas/open/45_shared_prepared_aggregate_stack_source_authority.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Add Shared Prepared Aggregate Authority

# Current Packet

## Just Finished

Step 2 - Add Shared Prepared Aggregate Authority completed for the concrete
aggregate `StackSlot -> Register` publication family.

Shared prepared lookup now exposes `PreparedAggregateStackSourceAuthority` on
each available `PreparedEdgePublication`, plus
`prepare_aggregate_stack_source_authority(...)` for focused lookup use. The
carrier records the concrete candidate facts currently shared: source and
destination value ids/names, source and destination scalar type views, source
stack slot id, stack byte offset, stack size/copy width, stack alignment,
destination storage kind, and destination register placement when move
authority provides one.

The concrete aggregate `StackSlot -> Register` family now fails closed with
explicit `MissingAggregateCopyAuthority` once stack source facts, register
destination storage, OutOfSsa move authority, and destination placement are
present. Scalar I32/I64 stack-source paths stay outside aggregate authority
(`Unavailable`), and malformed aggregate candidates distinguish
`IncompleteConcreteStackSource`, `UnsupportedMoveAuthority`, and
`IncompleteDestinationMapping`.

Target aggregate lowering remains disabled. RISC-V focused coverage now checks
that signed-12 and large-offset `size_bytes == 16` stack sources expose the
shared missing-authority status and candidate copy facts, while the target
still returns `UnsupportedSourceHome` and records no scalar stack-load
provenance.

Missing upstream producer facts are still: destination lane mapping, lane
widths/byte offsets, ABI layout reference, partial-copy validity, and scratch
ownership expectations. These fields remain false/unavailable in the shared
authority carrier, so targets cannot recover aggregate copies from byte size,
offsets, fixture names, value ids, stack slot ids, or register placement width.

## Suggested Next

Step 3 of `plan.md`: only consume aggregate `StackSlot -> Register` sources in
a target after a real shared producer fills the missing lane mapping, ABI
layout, partial-copy, and scratch ownership fields; otherwise keep target
consumers fail-closed.

## Watchouts

Do not treat `MissingAggregateCopyAuthority` as permission to lower. It is an
observable fail-closed producer gap, not an aggregate-copy contract. The
existing scalar I32 helper, RISC-V preserved I64 concrete path, and dynamic
LoadLocal I32 source-memory path remain separate and should not be generalized
from this status.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log`

Result: passed. Backend CTest reported 163/163 tests passing. Proof log:
`test_after.log`.
