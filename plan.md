# LIR Function Parameter Authority Publication Runbook

Status: Active
Source Idea: ideas/open/742_lir_function_parameter_authority_publication.md
Activated from: producer-authority blocker at idea 734 Plan Step 4.5

## Purpose

Repair definition-side logical parameter publication at the structured HIR-to-
LIR producer boundary, classify the logical-versus-ABI relationship, and hand
only proven authority back to the parked new-BIR receiver initiative.

## Goal

Make declarations and definitions publish the same owned logical parameter
facts while preserving distinct ABI signature facts and never deriving
semantics from presentation text.

## Core Rule

Use HIR `Function::params` as logical authority and the existing
`populate_lir_function_params` helper as its LIR publication path. Names and
rendered signatures are presentation. Require exact three-track parity only
for shapes proven one-to-one; classify ABI-expanded shapes without flattening
or guessing them.

## Read First

- `ideas/open/742_lir_function_parameter_authority_publication.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`
- `src/codegen/lir/verify.cpp`
- `tests/frontend/frontend_lir_function_signature_type_ref_test.cpp`
- the current focused LIR producer tests and backend importer fail-closed proof

## Current Targets

- definition and declaration shells in
  `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`
- structured function-parameter verification in `src/codegen/lir/verify.cpp`
  only where an exact relationship is established
- focused frontend producer/verifier tests
- one concise authority/classification handoff under
  `docs/lir_function_parameter_authority/`

## Non-Goals

- no edits to new-BIR storage, builders, verifier, or `lir_to_bir` importer
- no parameter-body use identity, CFG, stack/local object, call, or return work
- no parsing or matching names, signature text, type spelling, or testcase
  identity
- no broad LIR redesign and no forced parity across ABI-expanded tracks
- no pointer, narrow, byval, aggregate, vector/HFA, va-list, function-pointer,
  or variadic receiver implementation
- no downstream target lowering, ABI placement, canonicalization, allocation,
  MIR, or emission

## Working Model

- `LirFunction.params` is the ordered logical parameter list.
- `signature_params` is the ordered fixed ABI signature list.
- `signature_param_type_refs` is the typed mirror of that ABI list.
- Plain fixed scalar rows are one-to-one across all three tracks.
- Explicit void is a logical sentinel plus the native signature void-list flag,
  not an ordinary ABI parameter.
- ABI-expanded rows may have different logical and signature shapes and must be
  classified explicitly.

## Execution Rules

1. Implement one bounded producer or verification contract per packet.
2. Start with the existing helper call in the ordinary definition shell; do
   not introduce a second logical-parameter builder.
3. Inspect structured fields directly in tests. Rendering may be observed only
   after authority is already proven.
4. Keep declarations behaviorally unchanged and prove definitions match them.
5. Do not edit the idea-734 receiver while this plan is active.
6. Reject malformed/mismatched authority through the reachable LIR verifier
   where the relationship is exact.
7. Keep ABI-expanded relationships truthful instead of forcing a scalar rule
   onto them.
8. Every code packet requires a fresh build, focused proof, neighboring
   negative proof, and the supervisor-selected regression checkpoint.

## Ordered Steps

### Step 1 - Publish definition logical parameters from HIR authority

Goal: close the exact production omission without changing schema or receiver
behavior.

Primary target:

- the definition shell in `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`

Actions:

- invoke `populate_lir_function_params` for every ordinary definition at the
  same ownership point used by declarations and before function-context/body
  lowering consumes the shell
- preserve owned `TypeSpec`, HIR order, and ordinary display-name generation
- add direct structured tests for empty, explicit-void, and unused fixed
  `int`/`long long`/`float`/`double` declaration/definition pairs
- prove exact order/count/type agreement across all three tracks for the plain
  scalar row and prove misleading names do not provide authority
- keep the new-BIR receiver unchanged and fail-closed during this producer
  packet

Completion check:

- a fresh build and focused producer proof show definitions and declarations
  publish matching logical facts; the four-scalar definition no longer has
  `params=0`; void/empty distinctions remain exact; and the existing backend
  boundary has not been relaxed

### Step 2 - Verify exact relations and classify non-one-to-one shapes

Goal: make the producer contract reject malformed exact relationships while
preserving truthful ABI distinctions.

Actions:

- audit the reachable LIR verifier for logical parameter ownership and the
  established plain one-to-one relationship
- add the smallest structured verifier checks needed for missing, reordered,
  or type-conflicting plain fixed parameter authority
- prove declaration/definition parity and misleading-display independence
- classify pointer, narrow integer, byval, HFA/vector/aggregate or other ABI
  expansion, and variadic fixed-prefix rows by logical carrier, ABI carrier,
  mirror, and receiver disposition
- do not force logical count/type equality when the ABI deliberately expands
  or transforms a parameter

Completion check:

- malformed one-to-one authority rejects through reachable verification,
  legitimate complex shapes retain their existing structured publication, and
  every classified row has a truthful supported-or-blocked disposition without
  receiver implementation

### Step 3 - Record the exact handoff to idea 734

Goal: give the parked consumer a checked, bounded authority contract.

Actions:

- create `docs/lir_function_parameter_authority/handoff_to_734.md`
- record zero/void, plain scalar, pointer, narrow, byval, ABI-expanded, and
  variadic rows with their logical/ABI/mirror authorities and exact receiver
  disposition
- name the exact plain fixed rows unblocked and the reasons other rows remain
  fail-closed
- confirm no display-derived authority or parallel parameter mapping was added

Completion check:

- the handoff can drive a bounded idea-734 receiver packet without re-deriving
  producer facts or broadening into unproven shapes

### Step 4 - Prove the producer boundary and decide closure

Goal: accept the decomposition only after focused and broader evidence agrees
with the source idea.

Actions:

- run a fresh build and focused frontend producer/LIR-verifier tests
- run the relevant backend fail-closed boundary proof without changing its
  expectations to hide missing receipt
- run the supervisor-selected broader/full regression proof
- audit the implementation, classification handoff, and Reviewer Reject
  Signals against the final diff
- ask plan-owner whether idea 742 is complete; if accepted, close it and
  reactivate open idea 734 at the exact parameter receiver row

Completion check:

- all source acceptance criteria have evidence, no reject signal is present,
  supervisor proof is accepted, and lifecycle can return to idea 734 without
  claiming broad parameter receipt

## Runbook Completion And Handoff

Runbook exhaustion alone does not close idea 742. Closure requires accepted
producer publication, verifier/classification evidence, the checked handoff,
and supervisor-owned broader proof. Idea 734 remains open and parked until that
decision.
