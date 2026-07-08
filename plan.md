# Prepared Value Freshness Authority MVP Runbook

Status: Active
Source Idea: ideas/open/587_prepared_value_freshness_authority_mvp.md

## Purpose

Make prepared/prealloc value freshness a first-class, queryable authority for
selected high-risk consumers instead of leaving each consumer to infer freshness
from homes, publication rows, preservation payloads, move bundles, or backend
fallback ordering.

Goal: implement the MVP freshness authority model, wire representative call
argument and move-bundle consumers through it, and prove that fresher producer
or publication sources outrank stale prior-preserved homes.

## Core Rule

Do not make freshness progress through expectation rewrites, unsupported-marker
changes, allowlist edits, named-testcase shortcuts, or backend-local ordering
only. Each code-changing step must either publish/query shared freshness facts
or make an existing consumer fail closed because the shared authority cannot
prove freshness.

## Read First

- ideas/open/587_prepared_value_freshness_authority_mvp.md
- docs/target_abi_contract_research/index.md
- docs/target_abi_contract_research/02_are_current_contract_fields_sufficient.md
- docs/target_abi_contract_research/03_where_target_facts_are_split.md
- docs/target_abi_contract_research/05_prior_preservation_freshness_and_stale_home_risk.md
- src/backend/prealloc/value_locations.hpp
- src/backend/prealloc/calls.hpp
- src/backend/prealloc/call_plans.cpp
- src/backend/prealloc/prepared_printer.hpp and implementation
- RV64 call/object emission consumers under src/backend/mir/riscv/
- AArch64 call consumers under src/backend/mir/aarch64/
- focused prepared/backend tests under tests/backend/

## Current Targets

- Prepared/prealloc freshness data model and lookup/query helper.
- MVP use kinds:
  - call argument source
  - move-bundle source
  - producer/publication operand for one supported prepared object or
    publication route
- MVP source kinds:
  - direct prepared value home
  - producer rematerialization
  - explicit publication
  - prior preservation
  - move-bundle source
  - ABI/formal home where already published by prepared facts
- First representative consumers:
  - prepared call argument source selection in src/backend/prealloc/call_plans.cpp
  - RV64 prepared/object call argument consumption that currently trusts local
    fallback ordering
  - prepared move-bundle source consumption where destination authority is
    currently stronger than source freshness authority

## Non-Goals

- Do not replace every prepared value-home, publication, preservation, and
  move-bundle API in this runbook.
- Do not build a global dataflow lattice for every value/use pair.
- Do not change target triple parsing, TargetProfile, or BIR semantic ABI
  classification.
- Do not implement uniform target physical register identity publication; that
  remains tracked by ideas/open/586_uniform_target_register_identity_policy.md.
- Do not broadly rewrite RV64, AArch64, x86, call lowering, or object emission.
- Do not remove existing fail-closed diagnostics until the freshness verifier
  covers the same malformed or unknown-authority shape.

## Working Model

Introduce a small prepared/prealloc authority record and query surface that can
answer, for one concrete use, which source is fresh enough to trust.

The model should distinguish:

- value identity: PreparedValueId plus prepared value name where available
- use context: MVP use kind and program point
- source kind: home, producer, publication, preservation, move source, ABI home
- proof kind: same-block-before-use, dominance or ordering, explicit
  publication, ABI/formal entry, call-boundary preservation, move-bundle
  authority, or fail-closed unknown
- source reference: storage, producer, publication, preservation, or move record
- precedence: producer rematerialization and explicit publication outrank older
  prior preservation when both are valid for the same value/use

Prefer names and storage placement that match existing prepared/prealloc style.
Do not invent a separate backend-local freshness model.

## Execution Rules

- Keep each implementation packet narrow enough for build plus focused tests.
- Preserve fail-closed behavior when no authority can be proven.
- Add debug/prepared dump visibility for selected authority facts before broad
  consumer migration.
- Prove nearby same-feature behavior, not only one named regression case.
- Escalate to supervisor/reviewer before accepting a slice that changes only
  expected output, unsupported markings, or backend-local fallback order.
- When a step changes shared prepared/prealloc structures, run the matching
  build and focused prepared/backend test subset before handoff.
- Keep closure inventory notes in todo.md during execution so final closure can
  name wired and intentionally unwired consumers.

## Step 1: Define Freshness Authority Model And Query Skeleton

Goal: add the shared MVP vocabulary and fail-closed query result before moving
consumers.

Primary targets:

- src/backend/prealloc/value_locations.hpp
- src/backend/prealloc/calls.hpp
- any existing prealloc lookup/query implementation file that owns value-home,
  call-plan, or move-bundle helper construction
- prepared printer/dump surface if needed for observability

Actions:

- Inspect existing prepared/prealloc naming, enum, lookup, and printer
  conventions.
- Add MVP enums or structs for use kind, source kind, proof kind, source rank,
  source reference, and query status.
- Add a shared query/helper entrypoint that returns either the selected
  freshness source or a precise fail-closed status.
- Make the helper deterministic when several valid sources exist for the same
  value/use.
- Add minimal dump/test visibility so selected authority facts can be asserted.

Completion check:

- The model is visible in code and either focused tests or prepared dumps.
- Query status distinguishes selected authority from fail-closed unknown.
- No backend consumer has been moved yet unless required for a compile proof.
- Build proof and a narrow prepared/prealloc test subset pass.

## Step 2: Publish Call-Argument Freshness Facts

Goal: publish freshness candidates from existing prepared call-plan facts
without inventing new semantic producers.

Primary targets:

- src/backend/prealloc/call_plans.cpp
- src/backend/prealloc/calls.hpp
- existing producer/publication lookup helpers used by call argument planning

Actions:

- Publish direct home authority only when the home is valid for the requested
  call-argument use.
- Publish producer rematerialization authority for existing explicit producer
  routes available to call arguments or publication paths.
- Publish explicit publication authority when the publication point proves
  current value availability for the use.
- Publish PriorPreservation authority only when it is unique, complete,
  dominance/position-valid, and target-verifiable.
- Rank valid producer rematerialization and explicit publication above older
  PriorPreservation for the same value/use.
- Keep unsupported or incomplete authority shapes fail-closed with narrow
  diagnostics or verifier statuses.

Completion check:

- Focused tests or dumps show producer rematerialization or explicit
  publication outranking PriorPreservation.
- Focused tests show PriorPreservation still accepted when it is the only
  complete valid source.
- Existing call-plan and value-home tests continue to pass.

## Step 3: Wire Representative RV64 Call-Argument Consumer

Goal: make at least one RV64 call-argument consumer consult the shared
freshness query before trusting a home or prior-preservation fallback.

Primary targets:

- RV64 call emission and object-emission paths under src/backend/mir/riscv/
- prepared call argument source consumption using PreparedCallArgumentPlan or
  PreparedCallArgumentSourceSelection

Actions:

- Identify the RV64 path that previously depended on local fallback ordering
  for stale-home-prone call arguments.
- Route that path through the shared freshness query for the relevant
  PreparedValueId/use.
- Preserve existing target validation for register bank, stack slot, and
  materialization payload completeness.
- Fail closed when the helper reports unknown, ambiguous, incomplete, or
  stale-authority status.
- Add focused coverage anchored in the stale-home evidence from closed ideas
  576 and 584 where practical.

Completion check:

- At least one RV64 call-argument path consumes the shared authority.
- Focused tests prove fresher producer/publication sources are used over stale
  preserved homes.
- Existing RV64 object/call tests in the relevant bucket continue to pass.

## Step 4: Publish And Consume Move-Bundle Source Freshness

Goal: stop treating destination bundle authority as implicit proof that a
source is fresh enough for MVP-supported move uses.

Primary targets:

- src/backend/prealloc/value_locations.hpp
- move-bundle lookup helpers and classification helpers
- RV64 or shared prepared move-bundle source consumers for stack destination or
  publication-related moves

Actions:

- Publish move-bundle source freshness only when the bundle has visible source
  authority for the use.
- Link the move source to the shared freshness query rather than only checking
  destination authority.
- Preserve the destination authority taxonomy repaired by prior work; this step
  consumes or links source freshness facts, not a broad taxonomy rewrite.
- Fail closed for move-bundle sources with missing, ambiguous, or incomplete
  source freshness authority.

Completion check:

- Focused tests prove a move-bundle source without sufficient source freshness
  fails closed instead of being accepted because the destination bundle is
  well-formed.
- Existing move-bundle and stack-destination authority tests continue to pass.

## Step 5: Add Producer/Publication Operand Coverage

Goal: cover one supported producer/publication operand path so the MVP is not
limited to call-preservation routing.

Primary targets:

- prepared publication lookup/query helpers
- prepared printer or dump output for publication-source authority
- one focused backend/prealloc test family for a prepared object or
  publication route

Actions:

- Select one supported route where producer or publication facts already exist
  and the MVP can prove freshness without new semantic producer analysis.
- Publish authority facts for that operand use.
- Add query assertions or prepared dump assertions for the selected source.
- Keep unwired nearby operand families listed in todo.md for closure inventory.

Completion check:

- The MVP supports a producer/publication operand use kind.
- Focused tests or dumps show the selected freshness authority.
- No unrelated publication route is silently claimed as covered.

## Step 6: AArch64 And Unwired Consumer Inventory

Goal: record which consumers use the MVP authority and which remain protected
by existing fail-closed checks or explicit deferral.

Primary targets:

- AArch64 call consumers under src/backend/mir/aarch64/
- x86/shared-prealloc consumers touched by nearby query surfaces
- todo.md closure inventory notes

Actions:

- Review AArch64 consumers near prior-preservation and call-argument payload
  completeness checks.
- Wire AArch64 through the shared helper only when it is narrow and supported
  by the MVP model.
- Otherwise document why the AArch64 path is deferred, already protected, or
  out of scope, and name the existing fail-closed diagnostic or test evidence.
- Review x86 and shared-prealloc nearby consumers for accidental reliance on
  stale homes.
- Keep the inventory concrete enough to satisfy closure note requirements.

Completion check:

- todo.md names each wired consumer and each nearby unwired consumer reviewed.
- Existing AArch64 fail-closed behavior is preserved.
- Any newly wired path has focused proof.

## Step 7: Acceptance Validation And Closure Inventory

Goal: prove the MVP acceptance criteria and prepare a concrete closure note.

Actions:

- Run the supervisor-selected focused backend/prealloc subset for call plans,
  value homes, move bundles, RV64 object emission, and any wired AArch64 path.
- Escalate to broader validation if shared prepared/prealloc structures have
  been changed across multiple consumers.
- Ensure tests cover:
  - producer rematerialization or explicit publication outranking
    PriorPreservation
  - PriorPreservation accepted when uniquely complete and valid
  - move-bundle source failure without source freshness authority
  - at least one RV64 consumer consulting the shared query
- Assemble closure inventory in todo.md:
  - implemented source kinds
  - supported use kinds
  - wired consumers
  - unwired RV64, AArch64, x86, and shared-prealloc consumers
  - reason and evidence for every deferred consumer
  - recommended follow-up ideas

Completion check:

- All MVP acceptance criteria from the source idea are satisfied or explicitly
  called out as not yet complete.
- Regression proof is fresh for the final slice.
- Closure notes are concrete enough for plan-owner close review.
