Status: Active
Source Idea Path: ideas/open/841_lir_compact_scalar_abi_leaf_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate named scalar producers and consumers

# Current Packet

## Just Finished

Completed plan.md Step 2 for idea 841. Added `LirCompactScalarType` as the
minimal compact scalar authority carrier/ref for true scalar integer widths and
floating builtins `half`, `float`, `double`, `fp128`, and `x86_fp80`; attached
it first to `LirBinOp` as a selected scalar-only mirror of `type_str`; and kept
the existing `LirTypeRef` field as the rendering/compatibility mirror.

Added verifier checks so an attached `LirBinOp.compact_scalar_type` must mirror
the selected scalar type exactly and rejects vector, aggregate, function,
opaque, pointer, and void families. Added focused `frontend_lir_call_type_ref`
coverage for positive integer and floating scalar binops plus those
wrong-family rejection cases.

## Suggested Next

Execute Step 3 with one bounded named scalar producer/consumer migration. A
coherent next packet is to migrate `LirBinOp` verifier/printer consumption from
`type_str` checks to `compact_scalar_type` for the selected scalar binop path
while keeping `type_str` as parity/rendering text and preserving unmigrated
schemas.

## Watchouts

Do not implement 734 Raw-BIR receiver work in this idea. Do not assume opaque is
scalar, do not parse rendered text as scalar authority, and do not reopen
accepted receiver rows such as `LirAbsOp` selected-global/i32.

The Step 2 carrier is intentionally optional for compatibility and auto-derived
from `LirBinOp.type_str` through aggregate initialization. Later Step 3 packets
should tighten named producers/consumers one group at a time rather than
requiring every raw/manual `LirBinOp` compatibility construction to migrate at
once.

## Proof

Step 2 implementation proof passed:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'; } > test_after.log 2>&1`.

Additional whitespace proof passed: `git diff --check`.

Proof log path: `test_after.log`.
