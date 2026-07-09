# Prepared Incoming Stack Formal Authority Runbook

Status: Active
Source Idea: ideas/open/652_prepared_incoming_stack_formal_authority.md

## Purpose

Publish the producer/prealloc authority that RV64 needs before it can consume
stack-passed scalar formal homes on the object route.

## Goal

Make the caller-stack incoming byte offset/address for one stack-passed scalar
formal an explicit prepared fact, or classify the smaller producer/prealloc
owner that blocks publication.

## Core Rule

Do not reconstruct incoming formal locations in RV64 from formal order, ABI
size/alignment, source syntax, final assembly, or `stack_frame_bytes`. The
incoming caller-stack authority must be explicit and distinct from the callee
local spill-slot/home.

## Read First

- ideas/open/652_prepared_incoming_stack_formal_authority.md
- ideas/closed/644_rv64_object_route_stack_parameter_abi_residual.md
- ideas/closed/512_stack_passed_parameter_home_publication.md
- review/reviewA.md

## Current Scope

- Refresh the prepared facts for `src/20001017-1.c` after idea 644.
- Identify where stack-passed formal publication is produced.
- Add or expose explicit incoming caller-stack formal authority in
  producer/prealloc data.
- Keep RV64 consumers fail-closed until that authority is present.

## Non-Goals

- Do not rebuild RV64 incoming offset inference.
- Do not treat `IncomingStackToHome` plus callee local home as the incoming
  caller-stack authority.
- Do not reopen branch clobber-safety, terminator fragments, runtime policy,
  expectation files, unsupported markers, allowlists, timeouts, or accounting.
- Do not add named-case handling for `src/20001017-1.c`.

## Working Model

`src/20001017-1.c` now stops at
`unsupported_param_home: RV64 object route requires explicit prepared incoming
stack formal authority before consuming stack-passed scalar formal homes`.
The missing fact belongs before RV64 consumption: it must name the incoming
caller-stack location for a stack-passed formal separately from the callee's
local home.

## Execution Rules

- Keep routine execution notes in `todo.md`.
- Start by proving whether the producer/prealloc layer already has a latent
  incoming stack formal offset/address that is not published to RV64.
- Prefer producer/prealloc focused tests before RV64 positive consumer tests.
- Add negative coverage for local-home-only facts, missing authority, and
  ambiguous incoming formal authority.
- Any code-changing acceptance slice needs fresh build proof and the
  supervisor-selected regression comparison.

## Steps

### Step 1: Locate The Producer Authority Boundary

Goal: identify the current producer/prealloc representation for stack-passed
scalar formals.

Actions:
- Inspect prepared dumps and producer/prealloc data for the failing
  stack-passed formal family in `src/20001017-1.c`.
- Distinguish `IncomingStackToHome`, callee local home, and any latent
  caller-stack incoming offset/address.
- Record whether the missing fact is a publication gap or an upstream
  computation gap.

Completion check:
- `todo.md` names the exact producer/prealloc owner and whether a latent
  explicit incoming authority already exists.

### Step 2: Publish One Explicit Incoming Stack Formal Fact

Goal: make one stack-passed scalar formal's caller-stack incoming location an
explicit prepared authority.

Actions:
- Add the smallest producer/prealloc representation that records the incoming
  caller-stack offset/address for the formal.
- Keep the fact separate from local spill-slot/home records.
- Expose the fact through focused dumps, lookup helpers, or contract tests
  used by downstream consumers.
- Preserve fail-closed behavior for missing or ambiguous authority.

Completion check:
- Focused producer/prealloc coverage proves complete explicit authority and
  negative local-home-only coverage.

### Step 3: Wire RV64 To Consume The Explicit Authority

Goal: let RV64 load stack-passed scalar formals only from the explicit
incoming authority.

Actions:
- Replace any remaining RV64 consumer expectation with consumption of the new
  prepared fact.
- Keep diagnostics for missing, ambiguous, or local-home-only authority.
- Add focused RV64 object coverage that traces the load source to the explicit
  prepared fact, not to a literal inferred offset.

Completion check:
- The narrow build/test proof passes, and `src/20001017-1.c` advances past the
  missing explicit incoming stack formal authority diagnostic.

### Step 4: Validate And Review Boundaries

Goal: prove the route is not testcase overfit or a revived RV64 inference.

Actions:
- Re-run the supervisor-selected RV64 object and producer/prealloc subsets.
- Compare before/after logs with the regression guard.
- Inspect nearby stack-passed scalar formal cases if shared ABI publication or
  RV64 consumer code changed.

Completion check:
- Regression proof is non-regressing, and `todo.md` records any remaining rows
  assigned to separate owners.

### Step 5: Final Lifecycle Review

Goal: decide whether the producer/prealloc authority idea is complete.

Actions:
- Compare final behavior against the source idea acceptance criteria and
  reviewer reject signals.
- Close only if explicit incoming stack formal authority is published and
  consumed without RV64 reconstruction, or if a more precise owner has been
  recorded with current evidence.

Completion check:
- Plan owner can close, deactivate, or split the lifecycle state with matching
  validation evidence.
