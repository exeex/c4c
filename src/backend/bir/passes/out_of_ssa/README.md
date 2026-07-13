# Out-of-SSA Pass

Status: scaffold (unimplemented).

## Owns

Lowering phi nodes or block arguments to generic edge-copy or parallel-copy
pseudo operations before the MIR boundary, while preserving CFG semantics.

## Does not own

MIR-side phi repair, register allocation, concrete move opcodes, critical-edge
semantics hidden outside BIR, or target-specific copy scheduling.

## Input

Verified pseudo BIR with explicit CFG authority and valid SSA/block-argument
relationships.

## Output

A revision with no unresolved phi/block-argument transport and with all edge
copies represented by admitted generic pseudo operations.

## Verification and publication gate

Publication must verify edge coverage, predecessor/successor agreement,
parallel-copy semantics, stable value identity, and absence of unresolved SSA
joins. MIR accepts no implicit out-of-SSA obligation.
