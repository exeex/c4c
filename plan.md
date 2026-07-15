# BIR NodeKind Tag Algebra and Phase Vocabulary Runbook

Status: Active
Source Idea: ideas/open/801_bir_node_kind_tag_algebra_and_phase_vocabulary_lowering_contract.md

## Purpose

Publish the closed `NodeKind` tag algebra and B-through-F vocabulary-lowering
contract that idea 732 and its phase children must consume, then prove the
single traits/schema authority with only the minimum justified C++ slice.

## Goal

Keep one flat `Node`/graph/arena model while making inheritance-like kind
classification, SSA terminology, phase admission, lowering transitions,
identity rules, and publication verification explicit and fail-closed.

## Core Rule

One hidden schema authority defines every kind-to-tag fact and derives both
compile-time and runtime queries. Documentation, verifiers, and passes may
consume that authority but must not create parallel tag tables, implicit
pass-through, or a second phase-specific node/storage model.

## Read First

- `ideas/open/801_bir_node_kind_tag_algebra_and_phase_vocabulary_lowering_contract.md`
- `ideas/closed/746_bir_node_kind_centric_storage_pass_contract.md`
- `ideas/open/732_bir_stage_document_convergence_umbrella.md`
- `src/backend/bir/core/README.md`
- `src/backend/bir/core/ir.hpp`
- `src/backend/bir/verify/verifier.cpp`

## Scope

- Audit the exact limitations left by closed idea 746.
- Publish one normative artifact at
  `docs/backend/bir_node_kind_tag_algebra_and_phase_vocabulary.md`.
- Define the closed tag axes, invalid combinations, static-versus-dynamic SSA
  boundary, public query API, B/C/D/E/F vocabularies, transition matrix,
  identity rules, and publication-verifier obligations.
- Only after the normative contract is reviewable, make a bounded C++ proof if
  needed to demonstrate one authority across semantic, prepared, pseudo, and
  machine representatives.

## Non-Goals

- Do not implement complete B, C, D, E, or F passes or enumerate the complete
  future kind vocabulary in code.
- Do not rewrite `Node`, graph, arena, operand/result storage, stable IDs,
  register allocation, MIR construction, target emission, or importer work.
- Do not resolve general result-vector normalization beyond the policy needed
  by this contract.
- Do not introduce TableGen, `.td`, generation, reflection, or another DSL.
- Do not edit/reopen idea 746 or revise/activate idea 732 in this lifecycle.

## Working Model

- A `NodeKind` belongs to a reviewed combination of tags across closed axes;
  an enum does not literally inherit.
- `SsaEligible` classifies a kind in a published SSA-governed vocabulary; it
  does not prove dominance, single definition, phi-edge, use-def, or graph
  publication validity. B4 owns that dynamic proof.
- B, C, D, E, and F publish explicit admitted vocabularies. Every accepted
  kind has an explicit retain/lower/expand/project/split/merge/disappear rule.
- Arena-slot stability is not semantic identity. Preserve `NodeId` only when
  operation/result identity, type, arity, roles, effects, and ownership remain
  exact.

## Execution Rules

- Complete normative taxonomy and transition design before deciding whether a
  C++ proof is necessary.
- Keep every axis finite and state exclusive, composable, and invalid
  combinations; reject a free-form boolean bag.
- Keep typelist/specialization/validation mechanics private to the schema
  layer and expose only small stable compile-time and runtime helpers.
- Reject unknown enum values, illegal stage admission, invalid tag
  combinations, descriptor drift, unhandled transitions, and catch-all
  pass-through.
- For any C++ proof, require a fresh build plus focused same-feature tests and
  a supervisor-selected broader checkpoint appropriate to shared backend code.
- Preserve unrelated user changes and do not weaken tests or verifier
  contracts.

## Ordered Steps

### Step 1 - Audit the closed-746 descriptor and stage contract

Goal: establish the exact current authority and the bounded delta owned by 801.

Actions:

- inspect the landed `NodeKindDescriptor`, helper API, validation, stage masks,
  verifier consumers, and nearby tests;
- inventory flat/duplicated facts, all-stage defaults, unknown-kind behavior,
  and current SSA/value/effect terminology;
- identify representative existing or contract-only semantic, prepared,
  pseudo, and machine kinds without inventing a full future vocabulary;
- record the precise normative artifact sections and any minimum C++ proof
  seam, while keeping proof implementation deferred.

Completion check: the audit names every closed-746 limitation that 801 must
resolve, the evidence locations, and a bounded artifact/proof seam without
expanding into phase-pass or storage work.

### Step 2 - Define the closed tag algebra and SSA boundary

Goal: make kind classification finite, reviewable, and semantically exact.

Primary target: `docs/backend/bir_node_kind_tag_algebra_and_phase_vocabulary.md`.

Actions:

- define the value, semantic-family, effect/control, stage-vocabulary,
  operand/result/type-policy, and MIR-realizability axes;
- list mutually exclusive groups, composable groups, required combinations,
  and invalid combinations with schema-review rules for extensions;
- distinguish `SsaEligible` from B4-established graph SSA validity;
- require stage-qualified queries or split Raw/Canonical kinds wherever a
  timeless static SSA answer would be false.

Completion check: all six axes and their composition rules are explicit, and
no `is_op_ssa`-style query can be read as dynamic graph proof.

### Step 3 - Specify the single query authority

Goal: define one schema source for compile-time and runtime classification.

Actions:

- specify hidden kind traits/tag composition and schema validation;
- define the stable pass-facing compile-time queries and runtime wrappers;
- define fail-closed behavior for unknown values, illegal stages, and schema
  divergence;
- choose the bounded semantic/prepared/pseudo/machine representative set that
  could prove the design without claiming full vocabulary implementation.

Completion check: the normative artifact shows how both query forms derive
from one authority, and no pass-visible typelist or parallel switch table is
required.

### Step 4 - Publish B-through-F vocabularies and transitions

Goal: make every phase boundary an explicit vocabulary-lowering contract.

Actions:

- define admitted input/output kind or closed-group sets for B, C, D, E, and F;
- complete B-to-C, C-to-D, D-to-E, and E-to-F rows with retained, added, and
  removed tags;
- state unchanged, lower, expand, project, split, merge, and disappear rules;
- name prerequisites, publication verifiers, and rejection of unknown,
  premature, stale, omitted, or unhandled kinds.

Completion check: each accepted input has an explicit transition outcome and
no row relies on broad family prose or implicit catch-all pass-through.

### Step 5 - Define identity and publication-verifier obligations

Goal: bind vocabulary lowering to the shared graph's semantic identity and
publication guarantees.

Actions:

- define exact `NodeId` preservation versus replacement conditions;
- define insertion, deletion, expansion, merge, and projection consequences
  for uses, provenance, revision keys, invalidation, and failure atomicity;
- define per-publication-stage vocabulary, payload, arity, role, result/type,
  effect/control, identity, and dynamic graph checks;
- make B4 SSA graph proof and all unknown/unhandled rejection explicit.

Completion check: every admitted lowering shape has a reviewable identity rule
and every publication stage has fail-closed verifier obligations.

### Step 6 - Review and publish the normative artifact

Goal: make the architecture a durable prerequisite for later idea 732 work.

Actions:

- review the artifact against every acceptance criterion and reviewer reject
  signal in the source idea;
- confirm it resolves flat descriptor facts, missing SSA/tag algebra, and
  all-stage legality rather than renaming them;
- confirm it names the stable API and exact B-through-F contract that 732 and
  its phase children must cite;
- decide from documented evidence whether the optional bounded C++ proof is
  necessary, and record that decision without modifying idea 732.

Completion check: the artifact is internally complete, traceable to 801, and
ready to serve as normative input; any C++ proof need is explicitly bounded.

### Step 7 - Land the bounded schema/query proof if required

Goal: demonstrate feasibility without implementing phase pipelines.

Actions:

- make only the minimum schema/query changes justified by Step 6;
- derive compile-time and runtime helpers from one authority across semantic,
  prepared, pseudo, and machine representatives;
- validate invalid combinations plus unknown-kind and illegal-stage behavior;
- add nearby multi-family coverage without encoding proof cases into
  production logic.

Completion check: a fresh build and focused schema tests pass, shared-code
broader proof is accepted by the supervisor, and the slice contains no full
phase vocabulary, pass, storage, allocation, MIR, importer, or 732 work. If
Step 6 proves code is unnecessary, record that evidence and mark this step
not required rather than creating ornamental implementation.

### Step 8 - Verify source completion and hand off

Goal: prove the contract is complete before lifecycle closure is considered.

Actions:

- map the artifact and any bounded proof to every source acceptance criterion;
- update the normative artifact's own status and completion handoff so it
  records the landed Step 7 proof instead of still claiming that proof is
  pending;
- inspect the final diff for duplicate authority, implicit pass-through,
  weaker contracts, testcase shaping, and scope drift;
- run documentation/structural checks and the supervisor-selected code proof
  ladder if Step 7 changed C++;
- hand the completed evidence to plan-owner for an explicit close decision;
  do not revise or activate 732 in this step.

Completion check: every criterion has accepted evidence, every reject signal
is absent, and plan-owner can close 801 without inferring completion merely
from runbook exhaustion.
