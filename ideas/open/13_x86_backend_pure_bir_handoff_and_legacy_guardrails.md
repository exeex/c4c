# X86 Backend Pure Bir Handoff

## Goal

Connect the x86 backend to the rebuilt backend pipeline through a pure BIR
handoff instead of the current mixed LIR/BIR transitional entry, and remove the
wrong boundary rather than preserving it as a legacy route.

The end state is:

- x86 codegen consumes canonical BIR or prepared-BIR data rather than
  re-deriving backend decisions from mixed LIR/BIR state
- the active x86 path uses the same backend contract family that prealloc,
  liveness, and regalloc are converging toward
- there is no active mixed-contract, legacy, or fallback x86 route competing
  with the canonical backend handoff

## Why This Is Separate

The current x86 backend sits on the wrong side of the backend boundary.

Recent backend work is making BIR carry the ABI, call-site, return, liveness,
and regalloc-adjacent information needed by downstream codegen. But the x86
entry path still behaves like a rewrite-in-progress bridge:

- direct BIR entry is rejected
- LIR entry still attempts mixed or transitional ownership
- x86 codegen owns local stack-space / regalloc integration that overlaps with
  the canonical prealloc route

That is not just ordinary implementation debt inside x86 emission. It is a
contract-boundary problem: the active backend pipeline can only converge if x86
stops depending on mixed upstream state and starts consuming the canonical BIR
side directly.

Keeping this as a separate idea makes the durable intent explicit:

1. define the correct x86 backend handoff boundary
2. connect x86 codegen to pure BIR or prepared-BIR input
3. delete or retire the wrong mixed ownership boundary instead of preserving it
4. prove the new handoff is the actual x86 route in use

## Scope

### 1. Pure BIR Handoff

- make the x86 backend accept the canonical backend-side input through pure
  `bir::Module` or the prepared-BIR handoff derived from it
- remove reliance on mixed LIR/BIR entry semantics for the active x86 path
- ensure the x86 emitter no longer rejects the canonical direct backend handoff
  merely because it is not the old transitional slice

### 2. Prepared Contract Consumption

- decide whether the x86 consumer boundary should be raw BIR plus local prepare
  or an already prepared backend module, and make that boundary explicit
- consume published backend metadata instead of re-inferring the same ABI or
  ownership facts in x86-local transitional logic
- align x86 call / return / parameter handling to the canonical backend ABI
  metadata where available

### 3. Route Cleanup

- remove old legacy entry or mixed fallback behavior instead of keeping it alive
  for compatibility theater
- delete or hard-retire code paths that keep the wrong LIR/BIR contract active
- make the canonical pure-BIR route the only x86 backend handoff that counts

### 4. Ownership Cleanup

- reduce or remove duplicated x86-local stack-space / regalloc integration that
  conflicts with the canonical backend prepare/prealloc route
- make x86 codegen consume the canonical ownership model for value locations,
  ABI moves, and frame decisions instead of maintaining a second shadow
  contract
- do not hide unresolved ownership conflicts behind a fallback, compatibility
  bridge, or second active path

### 5. Runtime Proof In Our Framework

- add or extend tests that prove the x86 backend route is entered from pure BIR
  or prepared-BIR input
- prove the active path uses canonical backend metadata and does not depend on
  a hidden mixed-contract route
- keep proofs semantic and route-oriented rather than text-shape based

## Core Rule

Do not accept a mixed contract as "good enough" once BIR already carries the
required backend facts.

This idea is only complete when the active x86 backend route both:

- consumes the canonical backend-side handoff
- is the only real x86 backend route for this handoff boundary

Compile success through a mixed, legacy, or fallback path is not sufficient.

## Constraints

- preserve the single active plan lifecycle; this idea should begin as a durable
  open idea, not an implicit plan switch
- do not preserve x86-specific fallback code just to keep the old mixed input
  limping along
- do not treat adapter survival as success if the active route still depends on
  the wrong contract boundary
- prefer explicit deletion-ready seams over hidden dual ownership

## Acceptance Criteria

- [ ] the active x86 backend route accepts pure BIR or prepared-BIR input as its
      canonical handoff
- [ ] x86 no longer depends on mixed LIR/BIR ownership for the active path
- [ ] canonical backend ABI and location metadata are consumed instead of
      re-derived in a conflicting local shadow pipeline
- [ ] the wrong mixed-contract x86 route is removed or hard-retired rather than
      preserved as fallback behavior
- [ ] tests prove the canonical x86 route is the actual route in use

## Non-Goals

- rewriting unrelated non-x86 backend families as part of this idea
- claiming success merely because `emit.cpp` accepts more inputs while the real
  codegen path still depends on mixed transitional state
- broad x86 backend completion beyond the handoff and ownership boundary
  defined here

## Good First Patch

Make the x86 backend boundary honest first: introduce a canonical pure-BIR
entry or explicit prepared-BIR adapter for the active route, delete the wrong
mixed handoff instead of preserving it, then add a bounded test that proves x86
consumes backend-published ABI/location information through the canonical
route.
