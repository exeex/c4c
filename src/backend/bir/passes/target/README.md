# Target-Specific Pseudo-BIR Passes

Status: scaffold (unimplemented and deferred).

## Owns

A future extension point for explicitly reviewed target-specific optimization
passes over pseudo BIR. Each pass must declare its prerequisites, preserved and
invalidated analyses, and verifier re-entry point.

## Does not own

A hidden target allocator, target-specific liveness/spill management, machine
instruction selection, silent mutation of published stages, or bypass of
shared allocation and `AllocatedBir` verification.

## Input

A verified pseudo-BIR stage and explicit profile-keyed target facts required by
the individual reviewed optimization.

## Output

A new immutable pseudo-BIR revision that remains within the admitted schema and
is eligible for the declared reverification path.

## Verification and publication gate

No pass is active merely because this directory exists. Any future pass must
be separately accepted, declare invalidation, and publish only after the full
applicable pseudo-BIR verifier gate succeeds.
