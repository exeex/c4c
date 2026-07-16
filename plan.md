# LIR Nominal Function Signature and Call Composition Runbook

Status: Active
Source Idea: ideas/open/839_lir_nominal_function_signature_call_composition.md
Activated after: closed 838 aggregate convergence route at commit `574f2a810`

## Purpose

Implement the F1 nominal-family owner for function signatures and calls after
the accepted A1 aggregate ref/store route, without absorbing body-use identity
or later nominal-family migrations.

## Goal

Replace partial function-signature text mirrors with a module-owned nominal
signature store that declarations and calls compose through legal family refs.

## Core Rule

Function signature identity is a structured module fact. Do not derive
semantic signatures from rendered strings, parsed call text, `signature_text`,
`args_str`, duplicate `arg_type_refs`, or testcase-shaped spelling.

## Read First

- `ideas/open/839_lir_nominal_function_signature_call_composition.md`
- `ideas/closed/838_lir_canonical_module_owned_aggregate_ref_store_convergence.md`
- `docs/lir_nominal_type_family_architecture/current_lir_type_ref_responsibility_matrix.md`
- `docs/lir_nominal_type_family_architecture/dependency_ordering.md`
- `docs/lir_nominal_type_family_architecture/closure_trace.md`
- `ideas/open/795_lir_body_parameter_authority_handoff.md`
- `ideas/open/829_lir_next_body_parameter_authority_handoff.md`
- `ideas/open/830_lir_direct_call_structured_argument_identity_prerequisite.md`

## Current Targets

- Add `LirFunctionSignatureRef` and its module store for return, ordered
  parameters, variadic shape, and ABI facts.
- Migrate declaration and call composition to consume nominal signature facts.
- Preserve raw extern and inline-asm compatibility as named one-way adapters
  until their consumers migrate.
- Delete semantic `signature_text`, `args_str`, parsed-call construction, and
  duplicate `arg_type_refs` only after all named consumers are migrated.

## Non-Goals

- Do not implement 829/830 native body-use or argument identity.
- Do not migrate scalar, vector, aggregate, union, global, universal-model, or
  producer families beyond the function-signature refs required here.
- Do not claim restricted value-carrier or unrestricted value-union completion.
- Do not weaken accepted aggregate-store owner checks from 838.

## Working Model

- The module owns function signatures through a stable ref, parallel to the
  accepted aggregate-store direction from 838.
- A signature contains the structured return family ref, ordered parameter
  family refs, variadic state, and ABI facts needed by declarations and calls.
- Calls and declarations compare/compose signature refs instead of reparsing
  display text or maintaining duplicate argument type mirrors.
- Actual argument/result value identity remains owned by 829/830 and later
  routes; this runbook can only use restricted carriers at the boundary where
  the source idea explicitly allows it.

## Execution Rules

- Keep each packet buildable and update `todo.md` with focused proof.
- Add store/schema first, then migrate one producer/consumer family at a time.
- Preserve malformed signature rejection, wrong-module aggregate alternatives,
  fixed and variadic behavior, printer visibility, and reference-collector
  coverage.
- Delete compatibility fields only when every named consumer has moved and an
  equivalent negative test proves stale text/mirror input is ignored or
  rejected.
- Escalate to a separate idea if implementation requires body parameter,
  argument value, scalar/vector/union, or generic value-carrier authority.

## Ordered Steps

### Step 1 - Establish the nominal function-signature store

Goal: introduce the smallest module-owned function-signature ref/store contract
without changing declaration or call semantics.

Actions:

- Inspect current declaration and call signature construction, verifier,
  printer, reference collector, and raw compatibility surfaces.
- Add `LirFunctionSignatureRef` and a module store for return type ref,
  ordered parameter type refs, variadic state, and ABI facts.
- Populate the store from existing structured family refs where they already
  exist, including aggregate refs from the accepted 838 route.
- Keep legacy mirrors as compatibility fields during this step.

Completion check: fresh build plus focused construction/verifier tests prove
one fixed signature and one variadic or rejected-malformed signature are stored
as module-owned facts, while wrong-module aggregate alternatives fail closed.

### Step 2 - Migrate declarations to signature refs

Goal: make function declarations/definitions compose and verify through
`LirFunctionSignatureRef`.

Actions:

- Route named declaration and definition producers through the signature store.
- Update declaration verifier logic to compare structured signature facts
  instead of `signature_text` or duplicate parameter mirrors.
- Preserve zero-parameter, explicit-void, fixed, variadic, aggregate
  return/parameter, and raw extern compatibility behavior.
- Add nearby positive and negative coverage for stale mirror disagreement.

Completion check: fresh build plus focused declaration lowering/verifier and
printer proof shows declarations consume signature refs and reject malformed or
wrong-module alternatives.

### Step 3 - Migrate call composition to signature refs

Goal: make direct call construction and validation consume nominal signature
facts without claiming argument value identity.

Actions:

- Route supported direct calls through the function-signature store.
- Replace parsed-call construction and `args_str` semantic use at migrated call
  sites with signature refs and named compatibility adapters.
- Keep actual argument/result value identity out of scope unless a restricted
  boundary carrier is already authoritative.
- Prove fixed, variadic, aggregate parameter/return, malformed, and raw-call
  compatibility cases.

Completion check: fresh build plus focused call lowering/verifier proof shows
calls compose legal signature refs, reject mismatches, and preserve raw
compatibility only as a named one-way adapter.

#### Step 3A - Repair variadic declaration admission before call migration

Goal: make backend LIR variadic declaration/import admission reach the
signature-ref call verifier path without making variadic text authoritative.

Actions:

- Inspect the monolithic `backend_lir` importer path that rejects variadic
  function declarations before typed-call verification runs.
- Route the existing variadic declaration shape through the module
  function-signature store, including structured variadic state and return
  extension attributes already present in the signature facts.
- Keep unsupported variadic body-use, va_list, and variadic argument value
  lowering outside this step; this step only admits the declaration/callee
  signature facts needed for call composition checks.
- Add or update nearby backend LIR coverage proving a variadic direct-call
  fixture reaches store-backed callee-signature validation and that malformed
  or stale variadic mirror disagreement still fails closed.

Completion check: fresh build plus focused `backend_lir` proof demonstrates a
variadic call-composition consumer resolving `callee_signature_ref` through the
module store before any retained signature/text fallback.

#### Step 3B - Add raw extern signature-store adapter entries

Goal: preserve raw extern direct-call compatibility as a named one-way adapter
that produces module function-signature facts for call verification.

Actions:

- Inspect raw extern declaration recording and direct-call lowering surfaces
  that currently lack module function-signature store entries.
- Add the narrow adapter from raw extern declaration facts to
  `LirFunctionSignatureRef` entries needed by migrated direct-call consumers.
- Keep global/extern initializer family migration and broad extern type-fact
  cleanup out of scope; this step only creates function-signature facts for
  raw extern call compatibility.
- Prove that raw extern direct calls resolve store-backed signature refs and
  that compatibility does not become a semantic text fallback after migration.

Completion check: fresh build plus focused backend LIR proof demonstrates raw
extern direct calls compose through signature refs while malformed or mismatched
extern signature facts fail closed.

#### Step 3C - Split byval store authority from retained ABI metadata

Goal: make fixed aggregate/byval call composition consume module signature-store
facts while preserving retained ABI-shaped metadata only for still-owned
verifier argument checks.

Actions:

- Inspect the fixed byval aggregate call verifier path where the signature
  store carries byval pointee/store facts but retained `callee_signature` still
  supplies ABI-shaped parameter metadata.
- Introduce the smallest explicit bridge or representation split that lets
  store facts be authoritative for call signature composition without deleting
  retained ABI metadata needed by existing verifier checks.
- Do not migrate aggregate producers, body parameter authority, argument value
  identity, or unrestricted value carriers; if that broader work becomes
  required, stop and return to plan-owner for a separate blocker idea.
- Prove store/retained disagreement, store/call parameter mismatch, and
  wrong-module aggregate alternatives continue to fail closed.

Completion check: fresh build plus focused backend LIR proof demonstrates a
fixed byval aggregate call composing through the module signature store, with
retained ABI metadata treated as verifier-local compatibility rather than
semantic signature authority.

### Step 4 - Migrate printer and reference collectors

Goal: make debug/printer/reference surfaces observe nominal signature facts
without recreating text authority.

Actions:

- Update printer/debug output to display stored signature facts while keeping
  text non-authoritative.
- Update reference collectors to traverse function-signature refs and their
  family refs directly.
- Add coverage for declarations and calls that include aggregate refs,
  variadic state, and malformed stale mirrors.

Completion check: fresh build plus printer and collector proof demonstrates
structured traversal and no semantic dependency on rendered signature text.

### Step 5 - Delete migrated mirrors and assess convergence

Goal: remove obsolete semantic mirrors after all named consumers have migrated
and record any downstream deletion gates.

Actions:

- Delete semantic `signature_text`, migrated `args_str` construction,
  parsed-call construction, and duplicate `arg_type_refs` where no remaining
  named consumer requires them.
- Preserve raw extern/inline-asm adapters only when still named and one-way.
- Run focused declaration/call/printer/collector proof plus the proportional
  shared-surface checkpoint.
- Return to plan-owner with exact unmet criteria if any deletion gate belongs
  to 829/830 or another later idea.

Completion check: source acceptance has supervisor-accepted proof that
declarations, calls, verifier, printer, and collectors consume signature/value
facts, or the route is repaired with exact remaining criteria.
