# Allocated BIR

Status: scaffold (unimplemented).

## Owns

The `AllocatedBir` (MIR-ready) capability/view and its verifier profile. It is
a verified view of the same immutable BIR revision, not a copied or second IR
graph.

## Does not own

Machine instruction selection, concrete target register spelling, frame-offset
encoding, assembler parsing, or late repair of incomplete allocation.

## Input

An admitted pseudo-BIR revision, verified pseudo-register layout, complete
allocation facts, and explicit spill/reload state.

## Output

A clean MIR-ready BIR view containing only admitted pseudo nodes plus
`InlineAsm`, with every required value assigned a legal pseudo home or covered
by explicit, consistent spill state.

## Verification and publication gate

Transactional publication must reject non-admitted nodes, missing or illegal
pseudo homes, unresolved register pressure, stale facts, inconsistent spill
slots, and malformed `Spill`/`Reload` placement. MIR may consume only the
verified capability.
