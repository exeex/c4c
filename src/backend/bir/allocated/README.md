# Allocated BIR

Status: scaffold (unimplemented).

## Owns

Transactional publication of the allocated revision as a `PreparedBir`
capability and its read-only `MirReadyBirView`. Both name the same immutable
allocated BIR graph and exact preparation/allocation/target keys; neither is a
copied or second IR graph. Names are prospective while this scaffold remains
unimplemented.

## Does not own

Machine instruction selection, concrete target register spelling, frame-offset
encoding, assembler parsing, or late repair of incomplete allocation.

## Input

An admitted allocated BIR revision, verified abstract-register layout,
revision-bound preparation and allocation facts, complete assignments, and
explicit spill/reload state.

## Output

`PreparedBir` plus a borrowed `MirReadyBirView` over the closed semantic node
table and allocator-created `Spill`/`Reload` nodes. Every allocatable value is
legally assigned at each register use; spill residency and reload coverage are
explicit. The view also exposes verified typed facts and the immutable
revision trace needed by MIR.

## Verification and publication gate

Transactional publication must reject non-admitted nodes, missing or illegal
abstract assignments, target opcodes, concrete registers, frame offsets,
unresolved register pressure, stale/mismatched keys, inconsistent spill slots,
and malformed `Spill`/`Reload` placement. MIR may consume only the verified
view. It verifies keys before selection, maps abstract slots through the exact
target layout, records a revision trace, and publishes target MIR only after
output verification; it does not rerun ordinary allocation.

Final backend spill/reload is legal only for a closed, bounded
selection/encoding constraint that the abstract schema cannot express. It must
prove the input was assigned, BIR capacity was not exhausted, and no missing
BIR spill is concealed; ordinary exhaustion is an upstream contract error.
