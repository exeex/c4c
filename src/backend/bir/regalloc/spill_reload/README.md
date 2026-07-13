# Spill and Reload

Status: scaffold (unimplemented).

## Owns

Pressure-driven spill choice, abstract spill-slot identity, and placement of
explicit pseudo `Spill` and `Reload` operations within BIR.

## Does not own

Concrete stack-frame offsets, target load/store encodings, MIR-side allocation
repair, or target-specific spill allocators.

## Input

The allocator's private candidate fork, revision-bound liveness/interference,
allocation pressure and eviction decisions, typed preparation facts, and the
verified abstract-register layout.

## Output

Candidate-local abstract spill-slot identities and explicit abstract
`Spill`/`Reload` nodes that make every transition between spill residency and
an assigned abstract home visible. Spill slots contain type/size/alignment but
never a concrete frame index or offset.

## Verification and publication gate

Publication must prove slot identity/type consistency, legal placement,
dominance and liveness coverage, and complete reloads before uses. No implicit
pressure spill may cross the allocated boundary. A spilled value has a
dominating `Spill`; every later register use is covered by an assigned
`Reload` result. Failure discards the private candidate and publishes no slot,
node, assignment fact, or new revision.
