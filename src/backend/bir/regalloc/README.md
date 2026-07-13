# Shared BIR Register Allocation

Status: converged design contract (unimplemented).

## E2 sole allocation authority

`E2` is the only ordinary pseudo-home allocator for RV64, AArch64, and x86.
It consumes one immutable reverified D5 or E3-retry revision, the exact E1
product for that revision, the exact `ProjectedConstraintSet` keyed to that
revision, and its reviewed
`VerifiedTargetLayout`. Target differences enter only through layout records
for abstract banks, alias units, eligible slots, legal groups, reserved units,
and call-preservation/clobber rules. There are no target-specific allocator
implementations or architecture-name policy branches.

E2 assigns finite abstract homes of the form `(category, class/group, slot)`.
It does not choose encoded machine names, stack displacements, instruction
encodings, frame layout, or parse inline-assembly template bytes. Later MIR
construction only maps a verified assignment; it cannot repair or replace
ordinary allocation.

## One legality relation

One shared legality predicate governs initial choice, coalescing, eviction,
retry, and final verification. It requires:

- a value's typed class and category to match an eligible layout slot;
- every multi-slot group to use one reviewed legal, width-correct, aligned
  group whose alias units are simultaneously free;
- assignment-equality ties to receive the same compatible home without
  merging their value identities;
- interference and early-clobber exclusions never to alias;
- inline-assembly clobbers and call-clobbered units to exclude values live
  across the boundary unless explicit verified preservation makes the
  interval discontinuity visible; and
- D5 copy bundles to retain simultaneous read-before-write behavior; legal
  coalescing may remove a later move but may not change edge coverage or
  invent a temporary; and
- every `CopyScratch` reservation to receive one finite legal home. The
  reservation is non-spillable and aliases neither the component's transferred
  homes nor any other scratch home needed simultaneously. Scratch reservations cannot be
  coalesced with transferred values, evicted into spill state, or omitted
  because a particular scheduling order appears to avoid them.

The allocator uses a deterministic order derived from stable identities and
reviewed layout order. Eviction requeues the displaced value and records the
rejected `(value, home, conflict-set)` choice. A choice cannot be retried with
the same state. Because values and eligible homes are finite, an E2 attempt
either assigns every pending value, returns one deterministic E3 spill request,
or reports that no legal assignment/spill route exists.

An E2 request may name only a spill-eligible ordinary allocation identity;
`CopyScratch` is never such an identity. If ordinary E3 retries cannot produce
a candidate with legal homes for the complete reservation set, or if any
required scratch home aliases a protected transfer or simultaneous scratch
home, E2 fails closed. It cannot ask D5 or MIR to create scratch capacity or
repair the alias conflict.

## E1/E2/E3 retry protocol

E2 never mutates its input. On capacity failure it returns a typed spill
request naming the exact revision, E1 key, pressure point, conflicts, and a
spill-eligible original allocation identity. E3 may apply that request only to
a private fork of the named revision. After E3 inserts explicit spill state,
the candidate advances its stage revision and invalidates CFG/def-use facts,
E1, E2, and the predecessor constraint projection and realizability facts. E3
must invoke the shared projection authority before reverification; the runner
then recomputes affected structural facts, fully reverifies the complete
candidate, freezes it, recomputes E1, and starts a fresh E2 attempt. No E1
fact, rejected-choice set, or partial assignment crosses the revision change.

Termination is explicit. Each successful E3 rewrite permanently marks one
previously unspilled original identity spill-resident for this transaction;
that identity cannot be selected again, and reload results are not recursive
spill candidates. The retry budget is therefore at most the number of
spill-eligible original identities in the initial exact revision. Each E2
attempt also has the finite rejected-choice bound above. Budget exhaustion, a
request for an already-spilled identity, impossible reload pressure, or lack of
strict progress fails the whole transaction.

## Candidate gate

Before the later publication boundary, the allocation candidate must prove
that every allocatable identity is either assigned one legal abstract home or
is represented by verified explicit spill state whose every register-resident
use is reached through an assigned `Reload` result. It also proves complete
tie/group/clobber/call/copy legality, complete legal assignments for every
non-spillable `CopyScratch` reservation, absence of forbidden overlapping
alias units, exact E1/revision/layout/`ProjectedConstraintKey`, and no unassigned or
implicit-spill escape.

Only the stable post-E3 candidate and its exact current E1, E2, and E3 facts
may enter D5's subordinate `CopyResolutionTransaction`. E2 does not resolve or
schedule a `ParallelCopy`; its assignment product supplies the already-chosen
ordinary and scratch homes under which D5 must either emit directly realizable
`EdgeCopy` nodes or reject the complete candidate.

Because D5 resolution and E4 frame-action materialization advance the revision,
the predecessor assignment product is never current for the final output.
After projection and E1 recomputation on the materialized graph, E2's
allocator-owned assignment validator consumes those facts, the exact final
`ProjectedConstraintSet`, target/layout keys, `CopyResolutionFingerprint`,
`FrameActionFingerprint`, and the immutable predecessor assignment table.
It proves every ordinary value, reload result, resolved-copy role, and used
scratch endpoint remains assigned; reruns the same class/group/slot, alias,
tie, early-clobber, call-clobber, and interference legality relation; and
installs one new E2 `AssignmentKey` product keyed to the final materialized
`PipelineStageStamp` and exact `LivenessInterferenceKey`. This validation does not reallocate,
coalesce, change a home, evict, or issue a spill request. E4 frame-action nodes
have fixed ABI/frame roles and introduce no allocatable identity. Stable identities,
unchanged homes, and the copy preservation record alone cannot rekey the
predecessor assignment product. Failure aborts the enclosing atomic
publication transaction, leaves predecessor products immutable, and mints no
E4 capability.

Any stale key, incomplete assignment, missing spill transition, verifier
failure, or retry failure discards the entire private candidate and all E1/E2/
E3 products. No function subset, assignment table, spill identity, new
revision, or downstream capability is published.
