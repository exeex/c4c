# LIR-To-New-BIR Container And Import Completeness Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md
Reactivated from: completed structured-identity handoff in ideas/closed/741_lir_structured_operand_and_terminator_identity_decomposition.md

## Purpose

Resume the target-independent Raw-BIR receiving boundary at the exact
producer-authoritative function-body rows unblocked by idea 741, while
preserving all previously landed module, type, global, and signature work.

## Goal

Import every semantic fact that has structured typed LIR authority into one
verified Raw-BIR module without loss or partial publication. Keep rows whose
producer authority remains raw or monostate explicitly fail-closed until their
source identity is resolved by a separate initiative.

## Core Rule

Every successful row must map an existing typed LIR authority directly to one
typed Raw-BIR container, builder path, verifier rule, and transactional proof.
Never invent missing identity from display text, names, printer output,
testcase identity, or a field's position/order. Preserve position/order for its
native structured semantic role in operands, uses, indices, and source order.

## Read First

- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `docs/lir_structured_identity/handoff_to_734.md`
- `ideas/closed/741_lir_structured_operand_and_terminator_identity_decomposition.md`
- current new-BIR core/schema, builders, views, verifier, and importer owners
- `src/backend/bir/lir_to_bir/README.md` and
  `src/backend/bir/LEGACY_COVERAGE.md` as coverage evidence only

## Landed Progress

- Runbook Steps 1-3 completed the coverage foundation, typed module/type/value
  storage, globals, strings, externs, symbols, initializer topology,
  specialization metadata, intrinsic requirements, and direct scalar function
  signature receipt.
- Idea 741 completed an exhaustive 38/38 `LirInst` plus 6/6 `LirTerminator`
  authority audit and landed four bounded producer-side authority contracts.
- Current active new-BIR boundaries remain exact: store/load/GEP report
  `UnsupportedOrdinaryInstruction`; scalar value return reports
  `InvalidVoidReturn`. No receipt is implied by the producer handoff.

## Current Structured Handoff

The callable producer authority is limited to these exact rows:

- direct selected-global scalar integer `LirStoreOp`
- direct selected-global scalar `LirLoadOp`
- selected-global array-decay `LirGepOp`
- scalar integer value `LirRet`

All local/SSA pointer forms, other stores, other GEP producers, non-integer
returns, casts, arithmetic, calls, PHIs, vectors, varargs, allocas, memory
intrinsics, inline-asm binding identities, and raw CFG labels retain their exact
existing disposition. Do not receive them by parsing compatibility text.

## Execution Rules

1. Implement one exact handed-off receiver subrow per bounded packet.
2. Reuse one coherent BIR value/global/constant registry; do not add a
   producer-name side table or parallel value model.
3. Use `LirValueId`, `LinkNameId`, `LirIntegerImmediate`, `LirTypeRef`, and
   typed GEP indices as authority. Display may test misleading parity only.
4. Extend typed storage, builder/view, importer dispatch, and reachable Raw-BIR
   verification together; storage alone is not capability progress.
5. Preserve source order, result/use ownership, type agreement, and
   module-transactional publication.
6. Leave every unowned raw/monostate compatibility row fail-closed with a
   stable diagnostic.
7. Reject legacy resurrection, catch-all success, testcase dispatch,
   expectation weakening, and unsupported downgrade.
8. Every code packet requires a fresh build, exact focused proof, neighboring
   malformed cases, and the supervisor-selected regression checkpoint.
9. Keep routine progress in `todo.md`; alter this runbook or source idea only
   for a real route, scope, or proof correction.

## Ordered Steps

### Step 4.1 - Receive direct selected-global scalar integer stores

Goal: implement the smallest receiver-owned function-body subrow made
actionable by the structured-identity handoff.

Primary target:

- `LirStoreOp` with typed scalar integer `type_str`, `LirIntegerImmediate`
  value, and `LinkNameId` pointer resolving to exactly one imported global

Actions:

- add the minimal typed Raw-BIR `Store` payload and immutable view exposure
- reuse or extend the coherent receiver registry to map the source
  `LinkNameId` to its imported global identity and materialize the native
  integer immediate as an ordinary BIR value/constant
- preserve ordered operand/use edges, scalar type agreement, function/block
  ownership, and source order
- dispatch only the exact authoritative store shape; keep SSA/local pointers,
  non-integer values, and raw compatibility unsupported
- verify unresolved/ambiguous globals, malformed authority alternatives,
  out-of-range immediates, type mismatch, use ownership, duplicates, and
  whole-module rollback
- add neighboring positive, misleading-display, malformed, and transactional
  tests without matching the focused testcase name

Completion check:

- a fresh build and exact focused proof import the authoritative store from
  typed facts, all malformed neighbors reject, raw store forms remain
  fail-closed, and no failure publishes a partial Raw or Canonical module

### Step 4.2 - Receive direct selected-global scalar loads

Goal: import the handed-off direct global load using the same receiver identity
registry and ordinary result model.

Actions:

- add typed Raw-BIR `Load` storage, builder/view, verifier, and import dispatch
- resolve the `LinkNameId` global and register exactly one instruction result
  keyed by the current function's source `LirValueId`
- verify result uniqueness, type/use/global ownership, misleading display, and
  transactional failure
- keep local/SSA pointer and raw result forms unsupported

Completion check:

- authoritative direct global loads preserve typed global/result identities in
  a fresh build and neighboring malformed or raw shapes fail atomically

### Step 4.3 - Receive selected-global array-decay GEP

Goal: preserve the exact handed-off global array-decay address path and ordered
typed indices.

Actions:

- add typed Raw-BIR `GetElementPtr` storage, builder/view, verifier, and import
  dispatch
- map the result `LirValueId`, base `LinkNameId`, element `LirTypeRef`, native
  `inbounds`, and ordered all-typed `LirGepIndex` values
- resolve integer immediates and current-function SSA indices through ordinary
  value authority without consulting display aliases
- reject mixed raw indices, unowned bases/results, malformed paths, type
  conflicts, and partial publication
- keep ordinary/raw GEP producers unsupported

Completion check:

- the exact selected-global array-decay subrow imports losslessly with ordered
  path identity; misleading text and all malformed/raw neighbors fail closed

### Step 4.4 - Receive scalar integer value returns

Goal: wire the handed-off scalar return value into the already-typed function
signature and existing Raw-BIR return terminator.

Actions:

- materialize `LirIntegerImmediate` or look up current-function `LirValueId`
  through the coherent receiver value registry
- set `ReturnTerm::value` and enforce exact agreement with the imported
  function return type
- verify missing/extra value, unknown/cross-function ID, unsupported authority,
  immediate range, type mismatch, and rollback
- retain existing void return receipt and leave non-integer/raw return forms
  unsupported

Completion check:

- authoritative scalar integer returns and void returns import correctly in a
  fresh build; malformed, misleading, and unowned forms reject atomically

### Step 4.5 - Complete remaining functions, signatures, CFG and local objects

Goal: finish the remaining structured function surface without treating the
four handed-off rows as whole-modern-LIR readiness.

Actions:

- implement typed parameter/result/signature, block/edge, stack-object,
  hoisted-alloca, and function metadata storage plus builders/views where
  structured source authority exists
- import arbitrary valid block order and preserve entry, edge, source-order,
  local-object, and lifetime ownership
- verify references, signature/CFG shape, local ownership, duplicates, and
  transactional rollback
- record a separate producer-identity initiative when a required source row is
  still raw; do not parse it or silently expand idea 734

Completion check:

- fresh build and exact function/CFG/object proof pass for every structured
  row, and all remaining raw compatibility is explicitly fail-closed

### Step 5 - Complete ordinary instruction semantic families

Goal: give every structured-authority `LirInst` alternative a typed new-BIR
node, ordinary operand/result wiring, and verifier coverage.

Actions:

- implement coherent instruction-family packets in dependency order
- preserve opcode, typed operands/results, effects, object/symbol references,
  metadata, and source order directly from typed LIR
- create a separate producer-identity initiative for required raw rows rather
  than deriving semantics from text
- extend builder/view/verifier/importer and nearby positive/negative tests

Completion check:

- every current instruction row has an exact supported typed path or a durable
  blocked-source disposition; fresh family and accumulated Steps 2-5 proofs pass

### Step 6 - Complete terminators and structured inline-assembly transport

Goal: complete structured terminator receipt and preserve inline-assembly facts
without a parallel value or target-interpretation system.

Actions:

- implement structured return, conditional/unconditional branch, switch,
  indirect branch, unreachable, and other actual terminator rows
- verify targets, edge arguments/results, successor identity, and malformed CFG
  atomically
- preserve opaque byte-exact assembly text and ordinary operand/result identity
  without parsing, allocation, or target preparation
- keep raw compatibility fail-closed and separate any missing producer identity

Completion check:

- all structured terminator and inline-assembly variants have neighboring
  positive/negative proof and no target interpretation enters Raw BIR

### Step 7 - Integrate the dispatcher, verifier and build boundary

Goal: prove all landed semantic families form one production importer and one
reachable verification/publication boundary.

Actions:

- remove bounded-surface rejections only for rows with typed receipt and proof
- integrate migrated importer owners exactly once without legacy authority
- prove deterministic diagnostics and whole-module rollback across early,
  mid-module, and final-verifier failures
- reconcile the coverage ledger with callable implementation and explicit
  blocked-source dispositions

Completion check:

- accumulated broader proof passes and no accepted row exposes partial state or
  relies on display-derived semantics

### Step 8 - Prove lossless completeness and transactional publication

Goal: close idea 734 only on exhaustive structured coverage and explicit
disposition of every remaining source-authority gap.

Actions:

- re-enumerate actual LIR variants and metadata against the final ledger,
  destinations, dispatcher, verifier, and tests
- prove representative structural parity without rendering as authority
- run the supervisor-selected fresh build, focused suite, and full regression
- audit scope, diagnostics, rollback, legacy quarantine, and forbidden
  downstream work

Completion check:

- every acceptance criterion is evidenced by callable code and tests, every
  current row has a truthful disposition, and the supervisor can decide idea
  734 closure without unsupported rows being misreported as capability

## Runbook Completion And Handoff

Completing Step 8 makes idea 734 eligible for closure; it does not resume Child
C automatically. Lifecycle then returns to idea 732 and reruns documentation
convergence against the landed implementation from phase A onward.
