# BIR Cutover And Legacy Cleanup

Status: Open
Last Updated: 2026-04-01

## Goal

Make the BIR pipeline the default backend flow, remove the old legacy path, and
clean up transitional compatibility code once the new path has sufficient
coverage and confidence.

This is the final consolidation phase after:

1. the BIR scaffold exists
2. the flag-gated BIR path has broad enough coverage
3. the backend test strategy has been upgraded to support the new architecture

## Why this plan

Parallel old/new execution is the right migration strategy early on, but it
should not become permanent technical debt.

Once BIR is credible and well-covered, the codebase should converge on one
backend path:

- one default lowering route
- one primary IR for backend work
- one clear test story

This final idea is about finishing the migration instead of carrying legacy
compatibility indefinitely.

## Scope

### 1. Flip the default

- make BIR the default backend flow
- preserve a temporary fallback knob only if needed for short-term safety
- remove developer confusion about which path is authoritative

### 2. Remove legacy backend-path plumbing

Candidates include:

- legacy adapter-first lowering entrypoints
- backend-side syntax parsing helpers that only existed for the old path
- old compatibility wrappers between LIR and backend IR
- stale debug/print code that only served the transitional architecture

### 3. Rename and clean the surviving concepts

After cutover, the surviving pipeline should read clearly as:

- HIR
- LIR
- BIR
- target backend emission

Any remaining `backend_ir` naming that refers to the old transitional model
should be cleaned up or retired.

This is also the phase where transitional names introduced or tolerated in `01`
should finally disappear where practical, for example:

- `lower_to_backend_ir(...)` becoming a BIR-named lowering entrypoint
- `src/backend/ir.hpp` either being retired, renamed, or reduced to a
  compatibility shim
- comments/docs treating BIR as the authoritative backend IR name

### 4. Finish test migration

- make the BIR-oriented layered test structure the primary backend test path
- keep only the legacy old-path tests that still have real value during the
  final fallback window
- remove obsolete transitional-path coverage
- ensure the surviving test layout still preserves clear responsibility
  boundaries such as lowering vs validation vs target codegen vs pipeline

## Preconditions

This idea should not activate until:

- BIR path is broad enough to cover the production backend flow
- old/new routing has already been exercised at scale
- backend tests can clearly validate the new default behavior

## Constraints

- do not cut over before coverage is strong enough
- do not preserve large amounts of dead transitional code "just in case"
- keep the fallback window short if a temporary escape hatch is retained

## Acceptance Criteria

- [ ] BIR is the default backend flow
- [ ] old legacy backend-path plumbing is removed or reduced to a short-lived
      emergency fallback
- [ ] transitional `backend_ir` / adapter naming is cleaned up where practical
- [ ] backend tests primarily validate the BIR-based path through the new
      layered test framework
- [ ] legacy-only code and tests are materially reduced

## Non-Goals

- introducing another backend IR abstraction after BIR
- keeping two equal-status backend paths indefinitely
- preserving transitional code longer than needed

## Good First Patch

After BIR coverage reaches the threshold set by the previous idea, flip the
default backend routing to the BIR path behind a temporary escape hatch and
start deleting legacy-only lowering/plumbing that is no longer exercised.
