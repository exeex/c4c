# LIR Remaining Ordinary Value Identity Publication Runbook

Status: Active
Source Idea: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Activated from: repeated producer-identity blocker after accepted idea-734 Plan Step 5.1

## Purpose

Decompose the remaining production ordinary instruction result/use identity
family into checked rows and focused probes, publish native authority through
the existing LIR value model, and hand only proven receiver rows back to parked
idea 734.

## Goal

Give every in-scope production ordinary result and use a stable typed
`LirOperand` authority allocated or resolved before rendering, with reachable
ownership/type verification and no parallel value model.

## Core Rule

Native authority precedes presentation. Allocate results through the owning
function's `LirValueId` path, propagate the exact ID to SSA uses, and preserve
native immediates and `LinkNameId` uses. Never recover identity from `%t*`
spelling, formatted operands, printer output, type text, or testcase identity.

## Read First

- `ideas/open/744_lir_remaining_ordinary_value_identity_publication.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `ideas/closed/741_lir_structured_operand_and_terminator_identity_decomposition.md`
- `docs/lir_structured_identity/authority_matrix.md`
- `docs/lir_structured_identity/handoff_to_734.md`
- `src/codegen/lir/ir.hpp`, `src/codegen/lir/verify.cpp`, and ordinary
  HIR-to-LIR expression/call producers
- `src/codegen/lir/call_args_ops.hpp` and
  `src/codegen/lir/hir_to_lir/call/target.cpp`

## Current Evidence

- Idea 734 direct zero-argument void Call receipt is accepted at commit
  `49ed1b386` with 3033/3033 monotonic proof.
- Scalar-result call production uses `fresh_tmp(ctx)`, so the result retains
  display spelling without `LirValueId` authority.
- Structured call arguments are currently rebuilt from formatted operand
  strings, so scalar SSA arguments lose stable current-function identity.
- Authoritative production `fresh_value(ctx)` use is currently bounded to the
  selected-global load/GEP paths completed by idea 741.
- Closed idea 741 explicitly leaves other ordinary rows raw or unclaimed; this
  plan extends that model without reopening its four completed contracts.

## Current Targets

- checked remaining-row authority matrix under
  `docs/lir_remaining_ordinary_value_identity/`
- four focused one-contract C probes under `tests/backend/case/`
- ordinary HIR-to-LIR call/expression producers
- existing `LirOperand`, `LirValueId`, typed-immediate, and `LinkNameId`
  carriers
- reachable LIR ownership/type verification and focused frontend tests
- final checked handoff to idea 734

## Non-Goals

- no new-BIR opcode, container, builder, verifier, importer, or receipt work
- no reopening or weakening the idea-741 store/load/GEP/return contracts
- no string/name/result-spelling lookup or parallel value graph
- no target/ABI lowering, call placement/expansion, variadic lowering,
  canonicalization, allocation, MIR, emission, or assembler work
- no CFG or terminator target identity
- no stack slot, alloca, local-object, lifetime, or body-parameter identity
  unless a separate evidence-backed lifecycle decision changes scope
- no testcase-specific fields, named-case dispatch, expectation weakening, or
  supported-to-unsupported changes

## Working Model

- `LirValueId` is function-owned result/SSA-use identity.
- `LirIntegerImmediate` is native scalar immediate authority.
- `LinkNameId` is module-owned direct symbol/callee/global identity.
- `monostate` means no semantic authority and cannot support receipt.
- Display/kind text is presentation or diagnostics only after native authority
  exists.
- Stack/local-object and CFG/terminator target identity are separate families,
  not ordinary result/use work by proximity.

## Execution Rules

1. Do not edit producers before the remaining-row baseline and focused probe
   bind the row to one generic carrier/verifier contract.
2. Keep one primary contract per probe and one coherent generic seam per code
   packet.
3. Allocate native results before rendering and return the same operand to
   every downstream structured use.
4. Preserve the closed idea-741 paths and use them as regression neighbors.
5. Extend reachable verification with every producer seam: invalid, duplicate,
   missing, unknown, cross-owner, alternative, and type conflicts must reject.
6. Do not treat matrix edits, helper renames, or display parity as capability.
7. Classify unrelated raw rows truthfully rather than widening a packet.
8. Every code packet requires a fresh build, exact focused proof, neighboring
   malformed proof, and the supervisor-selected regression checkpoint.
9. Keep routine packet progress in `todo.md`; alter the source idea or runbook
   only for a real route/scope/proof correction.

## Ordered Steps

### Step 1 - Establish the remaining ordinary authority baseline

Goal: produce a checked execution baseline for every remaining production
ordinary result/use row before changing carriers or producers.

Primary target:

- `docs/lir_remaining_ordinary_value_identity/authority_matrix.md`

Actions:

- start from the final closed idea-741 38-variant matrix and current
  `LirInst` definition; do not regenerate or reopen its four completed rows
- enumerate every remaining modern production ordinary result and operand use
  individually, distinguishing producerless legacy alternatives
- record exact producer function, current carrier alternative, current
  `fresh_tmp`/`fresh_value` behavior, native type authority, verifier state,
  focused probe, dependency, and receiver disposition
- identify which rows can share generic result allocation or use propagation
  and which need a later separate semantic carrier
- classify stack/local-object, body-parameter, and CFG/terminator target
  identity as separate blocked families unless concrete evidence proves the
  ordinary carrier contract is production-complete for a named row
- confirm the four required seams below are independently executable and do
  not hide a larger prerequisite

Completion check:

- the checked matrix accounts for every remaining production ordinary
  result/use row without a catch-all, preserves all idea-741 dispositions, and
  gives each in-scope row one exact next action or truthful blocker

### Step 2 - Extract and bind four focused one-contract probes

Goal: turn the repeated family into independent producer contracts before
implementation.

Actions:

- create `lir_direct_scalar_result_call_identity.c` for native scalar call
  result identity
- create `lir_direct_void_immediate_arg_identity.c` for native scalar immediate
  call-argument authority
- create `lir_direct_void_ssa_arg_identity.c` for a call argument using the
  exact authoritative result of a selected-global load
- create `lir_scalar_ordinary_value_chain_identity.c` for one representative
  non-call result/use chain
- keep each file minimal and one-contract; retain larger integration cases only
  as later observations
- bind each probe in the matrix to its producer, exact carrier transition,
  verifier rejection obligations, and forbidden text fallback

Completion check:

- all four probes expose one stable first bad fact and are bound to distinct
  generic producer/verifier contracts without testcase-shaped implementation

### Step 3 - Publish direct scalar call result identity

Goal: replace scalar-result call display-only allocation with an owning native
result ID.

Actions:

- allocate the result through `fresh_value(ctx)` in the ordinary call producer
  before formatting
- store and return the same `LirOperand{LirValueId}` through the existing call
  result field and downstream expression path
- preserve void calls as empty/no-authority results
- verify result kind, uniqueness, current-function ownership, return-type
  agreement, malformed alternatives, and display independence
- keep new-BIR call-result receipt outside this idea

Completion check:

- the focused scalar-result call and neighboring result shapes publish native
  ownership and malformed/misleading variants reject without text recovery

### Step 4 - Publish direct void scalar immediate argument authority

Goal: preserve native immediate authority through structured call argument
construction.

Actions:

- pass the existing `LirIntegerImmediate` operand into the structured call arg
  rather than reconstructing it from formatted argument text
- preserve the native argument `LirTypeRef`, extension metadata, and source
  order without performing ABI placement
- verify immediate range/type/alternative agreement and malformed authority
- keep variadic, expanded, aggregate, and other unrelated argument shapes out
  of this packet

Completion check:

- the focused void immediate call carries a native typed immediate through the
  ordinary structured argument path and malformed neighbors reject

### Step 5 - Publish direct void scalar SSA argument authority

Goal: prove an already-authoritative ordinary result becomes the exact call
use without a name lookup.

Actions:

- propagate the selected-global load result `LirValueId` into the structured
  scalar call argument
- preserve current-function ownership, argument order/type, and exact use of
  the same ID
- reject unknown, duplicate, cross-function, type-conflicting, monostate, and
  formatted-only alternatives
- retain the closed global-load producer contract unchanged

Completion check:

- the focused load-to-call probe carries one exact source ID end to end and
  reachable verification rejects all ownership/type conflicts

### Step 6 - Prove a representative non-call scalar result/use chain

Goal: demonstrate that the ordinary carrier mechanism is generic beyond calls
and refine the remaining-row classification.

Actions:

- choose the smallest production scalar ordinary result/use pair justified by
  the Step 1 matrix
- allocate its result through the same owning value path and propagate the ID
  to one later ordinary use
- extend verifier coverage for opcode/type/ownership and malformed alternatives
- use the result to classify which neighboring ordinary rows share the exact
  mechanism and which need a distinct future packet

Completion check:

- the focused scalar chain preserves a native ID across two non-call ordinary
  operations and supports an evidence-backed remaining-row packet sequence

### Step 7 - Complete bounded generic ordinary producer seams

Goal: apply the proven mechanism to the remaining in-scope production rows in
coherent, independently verifiable packets.

Actions:

- group rows only when they share result allocation, operand propagation, type
  authority, and verifier rules
- implement one group per executor packet with focused neighboring proof
- keep rows needing opcode/predicate/object/aggregate/ABI-specific authority
  blocked until that additional contract is separately proven
- continuously update the checked matrix and preserve the four idea-741 rows
- do not absorb stack/local, body-parameter, or CFG/terminator target families

Completion check:

- every in-scope remaining production ordinary row has native result/use
  authority and proof, or an exact separately owned blocker with no false
  receiver-ready claim

#### Step 7.32 status — blocked external direct scalar-call prerequisite

The attempted direct, fixed, nonvariadic, zero-argument unresolved external
`double` call is blocked before verifier work. The plain `DeclRef` production
route records its `extern_decl_link_name_map`/`extern_decls` link-ID facts but
does not retain a native `FnPtrSig`; consequently it cannot produce the
structured fixed-empty signature or allocate the required
`fresh_value(ctx)` result `LirValueId`.

Do not relax or extend verifier acceptance for this route, and do not claim an
idea-734 receiver row is ready. The source-level producer prerequisite is
owned separately by
`ideas/open/746_lir_unresolved_external_direct_call_signature_result_authority.md`.
Step 7.32 may be retried only after that inactive initiative proves the native
signature and result-authority production path without text recovery.

### Step 8 - Audit verification and publish the idea-734 handoff

Goal: prove the producer boundary is complete and give the receiver exact
callable rows.

Actions:

- re-enumerate current ordinary variants/producers against the final matrix
- audit invalid, duplicate, missing, unknown, cross-owner, alternative, and
  type-conflict rejection through reachable verification
- create `docs/lir_remaining_ordinary_value_identity/handoff_to_734.md`
- name exact source fields, carrier alternatives, producer guarantees,
  verifier obligations, focused proof, and receiver work for every unblocked
  row
- keep all unproven, separate-family, and producerless rows explicitly
  fail-closed

Completion check:

- the matrix and handoff have no omitted production ordinary row, no display-
  derived authority, and enough exact detail for bounded idea-734 receiver
  packets

### Step 9 - Prove acceptance, close and resume idea 734

Goal: close the decomposition only after source acceptance and broader proof.

Actions:

- run a fresh build, focused producer/verifier tests, relevant backend boundary
  tests, and the supervisor-selected full regression checkpoint
- audit the final diff against every Reviewer Reject Signal
- ask plan-owner whether idea 744 is complete; do not infer completion from
  runbook exhaustion
- if accepted, close idea 744 and reactivate idea 734 at the first exact
  receiver row named by the handoff

Completion check:

- supervisor proof is accepted, every source criterion is evidenced, no reject
  signal is present, and lifecycle can resume idea 734 without broad ordinary
  receipt claims

## Runbook Completion And Handoff

Idea 734 remains open and parked throughout this producer initiative. Closing
idea 744 proves native ordinary result/use publication only; all new-BIR
containers and receipt remain with idea 734.
