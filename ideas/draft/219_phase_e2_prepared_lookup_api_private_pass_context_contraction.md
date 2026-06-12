# 219 Phase E2 prepared lookup API private/pass-context contraction

## Goal

Contract prepared lookup APIs that Phase E0/E1 prove no longer need to be
public durable surfaces, turning them into private pass context, compatibility
adapters, or deleting them when all public consumers and oracles are gone.

This phase is not broad `PreparedFunctionLookups` retirement. It is one lookup
group and one public boundary at a time.

## Prerequisite

Do not open this draft until E0 identifies E2 candidates and the relevant E1
semantic duplicate migration or equivalent proof has closed.

## Direction

For each candidate, separate:

- semantic ownership now held by BIR;
- prepared fallback that must remain public;
- target/prepared policy that must remain retained;
- pass-context delivery that can become private;
- printer/debug/oracle consumers that still block contraction.

## In Scope

- One `PreparedFunctionLookups` group at a time.
- One helper family or public API boundary at a time.
- AArch64 pass-context cleanup only when fallback and policy remain available.
- Compatibility adapters with explicit expiration criteria.

## Out Of Scope

- Aggregate `PreparedFunctionLookups` deletion.
- `PreparedBirModule` deletion or facade-only rename.
- Contracting printer/debug/oracle surfaces without E3 replacement.
- Cross-target wrapper contraction without E4 proof.
- Baseline refreshes or expected-string rewrites as proof.

## Expected Output

The closure note must contain:

- the exact public prepared API or lookup group touched;
- the consumer inventory before and after;
- why the remaining surface is private pass context, retained fallback, or
  deleted;
- proof that production, printer/debug, target-wrapper, oracle, and
  expected-string behavior is non-regressive;
- any E3/E4 blockers that still prevent further contraction.

## Reviewer Reject Signals

- Hiding an aggregate behind a new facade without reducing public consumers.
- Deleting fallback while absent/invalid/duplicate/mismatch cases still need it.
- Treating AArch64-only cleanup as cross-target readiness.
