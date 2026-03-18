# AST Refactor Plan

## Goal

Clarify the responsibility boundary of AST-side template preprocessing so it acts as a
seed builder for later reduction, instead of partially owning instantiation policy.

This plan assumes:

- AST-side consteval stays as-is for now
- current behavior should remain test-compatible during refactor
- we prefer incremental cleanup over a full architecture rewrite

## Current Problem

The current AST preprocessing stage does more than collect work items.

Today it is responsible for:

- indexing template function definitions
- indexing explicit specializations
- scanning call sites for direct template applications
- recording deduced template calls
- building `template_fn_instances_`
- running a local fixpoint for consteval-template-related instances
- indirectly deciding which template functions get lowered in Phase 2
- populating `HirTemplateDef.instances`

This mixes together three different concerns:

1. discovery
2. reduction scheduling
3. instantiation/materialization policy

That overlap makes later HIR reduction logic harder to reason about.

## Target Split

AST preprocessing should own only:

- template definition indexing
- explicit specialization indexing
- call-site discovery
- deduction / binding extraction
- creation of seed work items for later stages

AST preprocessing should not own:

- the authoritative set of instantiated functions
- fixpoint iteration for template reduction
- final decisions about which instantiations are lowered
- mutation of final template-instantiation results as policy

## Desired Pipeline

### Phase A: AST Discovery

Build lightweight indexes and collect template-related work candidates.

Outputs:

- `template_fn_defs_`
- `template_fn_specs_`
- discovered template application records
- deduced template binding info where available

Non-goals:

- no concrete instantiation ownership
- no local reduction loop
- no direct population of final instance results

### Phase B: Initial Seed Emission

Translate AST discovery results into a seed list for later processing.

Possible seed record fields:

- source template name
- resolved type bindings
- resolved NTTP bindings
- origin kind
  - direct non-template call site
  - deduced call site
  - deferred-from-template-body
- source location

This phase can still filter obviously invalid or incomplete work items, but should not
decide final lowering policy.

### Phase C: Reduction / Instantiation Driver

Consume the seed list plus HIR-discovered deferred work.

This stage should become the single owner of:

- dedup
- specialization key generation
- explicit specialization selection
- instance lifecycle
- instance metadata updates

The driver may still run from `lower_ast_to_hir()` initially, but its logic should be
centralized instead of split across AST collection and deferred HIR callbacks.

## Recommended First Refactor

The first refactor should not change behavior. It should only make roles explicit.

### Step 1: Rename the mental model

Treat current AST collection as "seed collection", not "instantiation".

Concretely:

- `collect_template_instantiations()` is really collecting seed candidates
- `collect_consteval_template_instantiations()` is collecting extra seed candidates
- `template_fn_instances_` currently behaves like both a seed list and a final result

That last point is the main thing to fix.

### Step 2: Separate seed data from realized instances

Introduce a separate container for AST-discovered work items.

Example direction:

- `template_seed_work_`
- keep `template_fn_instances_` only for realized instances

Even if both are temporarily populated from the same data, the distinction should exist.

### Step 3: Centralize instantiation bookkeeping

Move the following into one shared helper or registry:

- `has_instance`
- `record_instance`
- specialization key creation
- mangled-name creation for realized instances
- explicit specialization lookup
- updating `HirTemplateDef.instances`

AST collection and deferred HIR handling should both go through this shared layer.

### Step 4: Stop AST fixpoint from acting like the real reduction loop

The AST-side repeated pass for consteval-template-related collection should be reframed
as temporary seed expansion only.

Short term:

- keep it if needed for compatibility
- make its output feed seed work only
- avoid treating its result as final instance ownership

Long term:

- reduce or remove it once deferred paths cover the same cases reliably

## Proposed Responsibility Matrix

### AST discovery layer

Owns:

- "what template applications appear in source?"
- "what bindings can already be read from syntax?"
- "what explicit specializations exist?"

Does not own:

- "is this instantiation realized yet?"
- "should it be lowered now?"
- "is fixpoint complete?"

### Instantiation registry / driver

Owns:

- "has this instance already been realized?"
- "what is the canonical specialization identity?"
- "which body should materialize this specialization?"
- "how is instance metadata recorded?"

### HIR reduction layer

Owns:

- discovery of newly exposed deferred work from lowered bodies
- repeated completion of unresolved work
- diagnostics for irreducible deferred work

Does not need to own:

- AST-level eager consteval, for now

## What We Should Not Change Yet

To reduce risk, the first refactor should avoid:

- changing AST consteval behavior
- changing codegen behavior
- changing specialization naming
- changing test expectations
- removing deferred HIR instantiation

## Success Criteria For This Refactor

- the code clearly distinguishes seed discovery from realized instantiation
- AST preprocessing no longer looks like the owner of final template results
- deferred HIR instantiation and AST-derived work use the same bookkeeping rules
- behavior remains compatible with the current test suite

## Suggested Implementation Order

1. add this responsibility split as code comments near the collection pipeline
2. introduce a seed-work data structure without changing behavior
3. extract shared instantiation bookkeeping into one helper/registry
4. redirect AST collection and deferred HIR paths through that helper
5. only then consider shrinking AST-side iterative seed expansion
