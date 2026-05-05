# Member-Template Constructor Specialization Metadata Runbook

Status: Active
Source Idea: ideas/open/144_member_template_constructor_specialization_metadata.md
Activated From: ideas/open/141_typespec_tag_field_removal_metadata_migration.md Step 6 blocker

## Purpose

Split the repeated Step 6 constructor blocker out of the TypeSpec-tag deletion
route and repair the semantic carrier for member-template constructor
specialization.

## Goal

Selected constructor bodies must lower under both the instantiated owner record
and the selected constructor method-template bindings, including type packs and
NTTP packs.

## Core Rule

Do not make the known positive-Sema testcase pass through named-case matching,
rendered spelling recovery, or weaker expectations. Repair the constructor
member-template specialization carrier.

## Read First

- `ideas/open/144_member_template_constructor_specialization_metadata.md`
- `ideas/open/141_typespec_tag_field_removal_metadata_migration.md`
- `todo.md`
- `test_after.log`

## Current Targets

- Constructor selection metadata that connects generated specialization records
  back to primary record-member constructor templates.
- Constructor `Node` metadata carrying record-member template prelude and
  selected method-template information.
- Deduction paths for constructor call arguments and delegating constructor
  arguments.
- NTTP-pack binding from template patterns such as `index_sequence<I...>`.
- HIR lowering of selected constructor bodies under instantiated owner and
  method-template bindings.

## Non-Goals

- Do not edit TypeSpec-tag deletion scope except for reintegration validation
  after this carrier is repaired.
- Do not reintroduce `TypeSpec::tag` or rendered-string semantic lookup.
- Do not downgrade tests, mark supported paths unsupported, or rewrite
  expectations to hide the missing carrier.
- Do not broaden into unrelated LIR/BIR/backend/codegen work.

## Working Model

- The parent TypeSpec-tag deletion route is blocked, not complete.
- The first bad fact has moved from missing constructor lookup to missing
  method-template/NTTP-pack specialization.
- A generated specialization record may find the primary constructor body, but
  that is insufficient unless the selected lowering environment also carries
  method-template type-pack and NTTP-pack bindings.
- Focused probes are useful only when they isolate one of the carrier seams and
  preserve the existing positive-Sema contract.

## Execution Rules

- Start with a baseline command that proves the current 883/884 state.
- Inventory before changing code; identify which seam owns the first missing
  structured fact.
- Prefer the narrowest semantic carrier that composes through constructor
  selection, delegation, and HIR lowering.
- Keep implementation packets small enough to prove one seam at a time.
- Split a new open idea if investigation exposes an unrelated downstream
  carrier boundary.

## Step 1: Establish Positive-Sema Baseline

Goal: preserve the current failure family before changing implementation.

Primary Target: `test_after.log` and the positive-Sema subset.

Actions:
- Run `ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_'`.
- Confirm the baseline remains 883/884 with only
  `cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp`
  failing.
- Record the exact command and result in `todo.md`.

Completion Check:
- `todo.md` records the baseline command, pass/fail count, and the single
  failure name.

## Step 2: Inventory Constructor Specialization Seams

Goal: map the separable specialization seams before implementation.

Primary Target: parser/Sema constructor selection, constructor `Node`
metadata, template deduction, and HIR constructor lowering paths.

Actions:
- Trace how generated specialization records point back to primary
  record-member constructors.
- Identify where constructor `Node`s lose record-member template prelude
  metadata.
- Identify where constructor and delegating constructor arguments should drive
  method type-pack deduction.
- Identify where `index_sequence<I...>` or equivalent patterns should bind
  NTTP packs.
- Identify where HIR lowering chooses the constructor body and enters the body
  without the selected method-template bindings.

Completion Check:
- `todo.md` lists the observed source surfaces and names the first seam to
  implement.

## Step 3: Add Focused Probe Coverage If Needed

Goal: prove a separable seam without overfitting the final fixture.

Primary Target: frontend or HIR probes near the smallest missing carrier.

Actions:
- Prefer existing tests if they already expose the chosen seam clearly.
- Add a focused probe only when it distinguishes constructor prelude, method
  type-pack deduction, NTTP-pack binding, or selected-body lowering.
- Keep the final positive-Sema testcase as an end-to-end proof, not the only
  evidence.

Completion Check:
- The chosen proof command is recorded in `todo.md`.
- Any new probe fails for the missing semantic carrier before the code repair.

## Step 4: Implement Constructor Prelude Carrier

Goal: ensure constructor `Node`s preserve record-member template prelude when a
selected constructor comes from a member template.

Primary Target: constructor AST/Sema metadata handoff.

Actions:
- Thread existing structured record-member template prelude metadata onto
  constructor `Node`s.
- Avoid synthesizing identity from rendered constructor names or tags.
- Prove constructor lookup still reaches the intended primary constructor body.

Completion Check:
- Focused proof shows the selected constructor has structured prelude metadata
  available at the next deduction/lowering boundary.

## Step 5: Implement Method Type-Pack Deduction

Goal: bind constructor method type packs from constructor and delegating
constructor arguments.

Primary Target: template deduction for selected constructor calls and
delegating constructor calls.

Actions:
- Deduce packs such as `Args1` and `Args2` from the actual constructor
  argument lists.
- Preserve owner-record instantiation separately from method-template
  specialization bindings.
- Prove the selected constructor body no longer sees those type packs as
  unresolved template parameters.

Completion Check:
- Focused proof shows method type packs are bound before constructor body
  lowering.

## Step 6: Implement NTTP-Pack Binding

Goal: bind non-type template parameter packs used by constructor bodies.

Primary Target: deduction from patterns such as `index_sequence<I...>`.

Actions:
- Bind packs such as `I1` and `I2` from structured template argument patterns.
- Thread NTTP-pack bindings through the same selected method-template
  specialization carrier as type packs.
- Avoid case-specific `index_sequence` or `get` name shortcuts.

Completion Check:
- Focused proof shows calls such as `get<I>(args)` instantiate with concrete
  NTTP arguments instead of `I=?`.

## Step 7: Lower Selected Constructor Bodies Under Full Bindings

Goal: make HIR lowering enter the chosen constructor body with both owner and
method-template specialization state.

Primary Target: selected constructor body lowering environment.

Actions:
- Combine instantiated owner record metadata with selected constructor
  method-template type-pack and NTTP-pack bindings.
- Lower public and private/delegating constructor bodies through the same
  carrier shape.
- Verify constructed object type inference no longer collapses to `void`.

Completion Check:
- The positive-Sema subset no longer has the constructor specialization
  failure caused by unresolved `Args1`/`Args2` or `I1`/`I2`.

## Step 8: Reintegrate With Parent TypeSpec-Tag Validation

Goal: hand the repaired constructor specialization capability back to the
parent deletion route.

Primary Target: parent Step 6 validation entry point.

Actions:
- Rerun the supervisor-selected positive-Sema or TypeSpec-tag deletion subset.
- Record whether the parent Step 6 route can resume from
  `ideas/open/141_typespec_tag_field_removal_metadata_migration.md`.
- If a new unrelated boundary appears, create a separate open idea rather than
  absorbing it into this one.

Completion Check:
- `todo.md` records the reintegration proof and the recommended lifecycle
  action for the parent TypeSpec-tag deletion plan.
