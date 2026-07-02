# Pointer-Value Memory Provenance Publication Runbook

Status: Active
Source Idea: ideas/open/442_pointer_value_memory_provenance_publication.md
Activated From: post-517 return to remaining non-F128 provenance queue

## Purpose

Resolve the parked pointer-value memory provenance publication idea after the
closed-world authority prerequisite chain completed. The external-linkage
formal-pointer route must now consume the prerequisite outcome: no sound
no-external-caller metadata source exists in the current compiler model.

## Goal

Classify each residual `930930-1` pointer-value memory access as concrete
provenance, explicitly accepted opaque compatibility, or intentionally
unsupported, without weakening prepared authority or inferring provenance in
RV64 target lowering.

## Core Rule

Target lowering may consume only explicit prepared pointer-value memory
authority. Do not infer pointer ownership, object extent, layout, range, or
formal provenance from raw pointer values, integer casts, testcase names,
value names, object labels, or observed same-module callsites.

## Read First

- ideas/open/442_pointer_value_memory_provenance_publication.md
- ideas/closed/438_prepared_pointer_value_memory_authority.md
- ideas/closed/443_closed_world_formal_pointer_authority.md
- ideas/closed/444_no_external_caller_formal_authority_producer.md
- ideas/closed/445_closed_world_no_external_caller_metadata_source.md
- build/agent_state/438_step1_pointer_value_memory_audit/
- build/agent_state/443_step4_residual_disposition/
- src/backend/
- tests/backend/

## Current Targets

- Primary representative: `tests/c/external/gcc_torture/src/930930-1.c`
- Function of concern: external-linkage `930930-1::f`
- Residual patterns:
  - `%lv.param.reg1` pointer-value memory loads
  - pointer-delta shape such as `%mr_TR - 8`
- Existing authority contracts:
  - `prepare::prepared_pointer_value_memory_has_proven_authority`
  - `FormalPointerAuthorityKind::InternalOnly`
  - `FormalPointerAuthorityKind::NoExternalCaller` remains unpopulated after
    ideas 444 and 445

## Non-Goals

- Do not publish external-linkage formal pointer provenance from observed
  same-module direct callsites alone.
- Do not weaken `prepare::prepared_pointer_value_memory_has_proven_authority`.
- Do not infer layout, range, object extent, or provenance in RV64 from raw
  BIR, testcase names, function names, value names, integer arithmetic, or
  object labels.
- Do not implement pointer-delta propagation, including `%mr_TR - 8`, before
  base formal pointer provenance is authorized.
- Do not reopen store-source/global-memory publication, direct-global
  return/select-chain, terminator/select publication, pointer-cast movement,
  or selected global object data.
- Do not change expectations, unsupported markers, allowlists, runtime
  comparison, or pass/fail accounting.

## Working Model

- Idea 438 completed the prepared pointer-value authority contract and left
  unknown layout/range fail-closed.
- Earlier idea 442 work completed the internal-only same-module
  computed-address formal pointer provenance packet.
- Idea 443 added the formal pointer authority carrier and preserved
  external-linkage/no-proof fail-closed behavior.
- Idea 444 defined the no-external-caller contract but found no producer
  implementation in the current pipeline.
- Idea 445 rejected all current non-internal metadata sources for
  `NoExternalCaller`; `930930-1::f` remains `Unknown` and fail-closed.
- The remaining idea 442 task is therefore residual disposition and any
  separately covered opaque-compatibility policy that is semantically justified
  without target-side inference.

## Execution Rules

- Start by re-auditing the current `930930-1` prepared records against the
  idea 438 classifier and the closed 443/444/445 authority outcome.
- Treat the closed prerequisite result as durable input: external-linkage
  formal provenance is unavailable unless a future source idea provides a real
  metadata source.
- If no concrete provenance or named opaque policy is sound, record intentional
  unsupported disposition with focused proof.
- Keep any target-consumer work sequenced after producer authority is proven;
  do not bundle target inference with producer publication.
- For code-changing steps, run a fresh build, focused representative proof,
  focused backend coverage, and the supervisor-selected backend subset.

## Step 1: Rebuild Residual Provenance Evidence

Goal: Reconfirm the current `930930-1` pointer-value memory records and the
authority fields available after ideas 443, 444, and 445 closed.

Actions:

- Reproduce or inspect prepared dumps for `src/930930-1.c`.
- Record each pointer-value memory access, including `reg1`-based loads and
  `%mr_TR - 8` style pointer-delta rows.
- Record layout authority, range verdict, formal pointer authority kind,
  object extent, provenance source, and current diagnostic.
- Confirm which rows are internal-only eligible, external-linkage/no-proof,
  pointer-delta-later-work, or unrelated to this idea.

Completion check:

- `todo.md` records the residual row evidence and maps every representative
  to concrete provenance, possible opaque compatibility, or unsupported.

## Step 2: Decide Concrete, Opaque, Or Unsupported Policy

Goal: Decide whether any residual access can be published under existing
authority, needs a named opaque policy, or must remain unsupported.

Actions:

- Apply the idea 438 classifier to every residual access.
- Consume ideas 443/444/445 as the closed-world authority outcome.
- Define a narrow `OpaqueCompatibility` policy only if it can be covered
  semantically without target-side inference.
- If the route remains unsupported, make the owner and diagnostic precise.
- Keep pointer-delta propagation deferred until base formal pointer authority
  is soundly published.

Completion check:

- `todo.md` names the accepted or rejected disposition and any follow-up source
  idea required before target consumption.

## Step 3: Publish Or Preserve Fail-Closed Behavior

Goal: Implement the smallest producer/publication or diagnostic change needed,
or record that the current fail-closed behavior is the correct disposition.

Actions:

- If concrete provenance is available, publish prepared records that pass the
  idea 438 classifier.
- If an opaque policy is accepted, add the named predicate and focused tests.
- If unsupported is correct, preserve or refine fail-closed diagnostics without
  weakening authority checks.
- Do not add RV64 target-side inference.

Completion check:

- A fresh build passes when code changes are made.
- Focused proof shows the residual representatives either pass the authority
  classifier through explicit facts or reject with the intended owner.
- `todo.md` records the disposition and any remaining follow-up.

## Step 4: Add Focused Coverage

Goal: Cover the accepted or intentionally unsupported provenance disposition.

Actions:

- Add focused tests for concrete provenance or opaque compatibility if
  accepted.
- Add or preserve fail-closed coverage for external-linkage/no-proof formal
  provenance, unknown layout, unknown-compatible ranges, bare pointer
  identities, and pointer arithmetic.
- Keep tests semantic and independent of testcase names.

Completion check:

- Focused backend or prepared tests pass.
- Reject coverage proves missing authority remains fail-closed.
- `git diff --check` passes for touched files.

## Step 5: Validate And Handoff

Goal: Prove idea 442 is ready for closure or identify the exact remaining
producer prerequisite.

Actions:

- Run the build and focused tests required by the supervisor.
- Run the relevant backend/prepared subset for touched scope.
- Run regression guard when the supervisor asks for close readiness.
- Update `todo.md` with validation, residual risks, and whether source idea
  acceptance criteria are satisfied.

Completion check:

- Fresh validation is recorded in `todo.md`.
- The source idea can close as concrete, opaque-compatible, or intentionally
  unsupported, or the remaining work is separated into a new source idea.
