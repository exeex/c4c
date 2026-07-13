# Shared BIR Register Allocation

Status: converged design contract (unimplemented).

## E2 sole allocation authority

`E2` is the only ordinary pseudo-home allocator for RV64, AArch64, and x86.
It consumes one immutable reverified D5 or E3-retry revision, the exact E1
product for that revision, the exact `BoundConstraintSet`, and its reviewed
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
  invent a temporary.

The allocator uses a deterministic order derived from stable identities and
reviewed layout order. Eviction requeues the displaced value and records the
rejected `(value, home, conflict-set)` choice. A choice cannot be retried with
the same state. Because values and eligible homes are finite, an E2 attempt
either assigns every pending value, returns one deterministic E3 spill request,
or reports that no legal assignment/spill route exists.

## E1/E2/E3 retry protocol

E2 never mutates its input. On capacity failure it returns a typed spill
request naming the exact revision, E1 key, pressure point, conflicts, and a
spill-eligible original allocation identity. E3 may apply that request only to
a private fork of the named revision. After E3 inserts explicit spill state,
the candidate advances its stage revision and invalidates CFG/def-use facts,
E1, E2, and every dependent constraint projection or realizability fact. The
runner recomputes affected structural facts, fully reverifies the complete
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
tie/group/clobber/call/copy legality, absence of overlapping alias units, exact
E1/revision/layout/constraint keys, and no unassigned or implicit-spill escape.

Any stale key, incomplete assignment, missing spill transition, verifier
failure, or retry failure discards the entire private candidate and all E1/E2/
E3 products. No function subset, assignment table, spill identity, new
revision, or downstream capability is published.
