# Spill and Reload

Status: scaffold (unimplemented).

## Owns

Pressure-driven spill choice, abstract spill-slot identity, and placement of
explicit pseudo `Spill` and `Reload` operations within BIR.

## Does not own

Concrete stack-frame offsets, target load/store encodings, MIR-side allocation
repair, or target-specific spill allocators.

## Input

Pseudo BIR, shared liveness/interference facts, allocation pressure and
eviction decisions, and the verified pseudo-register layout.

## Output

Revision-bound abstract spill slots and explicit pseudo `Spill`/`Reload` nodes
that make every transition between memory state and pseudo homes visible.

## Verification and publication gate

Publication must prove slot identity/type consistency, legal placement,
dominance and liveness coverage, and complete reloads before uses. No implicit
pressure spill may cross the `AllocatedBir` boundary.
