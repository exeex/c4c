# LIR-To-New-BIR Container And Import Completeness Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md
Reactivated from: completed parameter-authority handoff in ideas/closed/742_lir_function_parameter_authority_publication.md

## Purpose

Resume target-independent Raw-BIR receipt at the exact function-signature row
unblocked by idea 742 while preserving all landed module, global,
instruction, and terminator work and keeping every unproven parameter shape
fail-closed.

## Goal

Import every semantic fact with structured typed LIR authority into one
verified Raw-BIR module without loss or partial publication. Record a separate
producer initiative whenever a required source identity remains raw rather
than reconstructing it from presentation.

## Core Rule

Every successful row must map an existing typed LIR authority directly to one
typed Raw-BIR container, builder path, verifier rule, and transactional proof.
Names, signature rendering, raw body operands, testcase identity, and ABI
position are never parameter identity or type authority.

## Read First

- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `docs/lir_function_parameter_authority/handoff_to_734.md`
- `ideas/closed/742_lir_function_parameter_authority_publication.md`
- `docs/lir_structured_identity/handoff_to_734.md`
- current new-BIR function/signature storage, builders, views, verifier, and
  LIR importer owners
- `src/backend/bir/lir_to_bir/README.md` and
  `src/backend/bir/LEGACY_COVERAGE.md` as coverage evidence only

## Landed Progress

- Runbook Steps 1-3 completed the coverage foundation, typed module/type/value
  storage, globals, strings, externs, symbols, initializer topology,
  specialization metadata, intrinsic requirements, and direct scalar return
  signature receipt.
- Steps 4.1-4.4 receive direct selected-global scalar integer stores, direct
  selected-global scalar loads, selected-global array-decay GEPs, and scalar
  integer returns through structured identity.
- Closed idea 741 supplied the exact instruction/terminator authorities for
  those four receiver rows.
- Closed idea 742 now publishes definition logical parameters, verifies proven
  plain three-track parity, and classifies every neighboring parameter shape.

## Current Structured Handoff

The first callable parameter receiver surface is exactly:

- empty zero-fixed-parameter declaration and definition signatures
- explicit-void declaration and definition signature shape
- default-shape nonvariadic fixed `int`, `uint`, `long long`, `unsigned long
  long`, `float`, and `double` declaration and definition signatures

The plain row has exact structured count, order, base/shape, nonbyval state,
and typed mirror parity across `LirFunction.params`, `signature_params`, and
`signature_param_type_refs`. Explicit void uses one logical `TB_VOID` sentinel,
no fixed signature rows or mirrors, and the native void-list flag.

Body parameter binding remains blocked because current body operands do not
carry native parameter value identity. `long` and `unsigned long` are also
blocked: production LIR and its verifier currently require 64-bit mirrors even
for I686, while new-BIR return/global type policy preserves 32-bit I686 long.
Open inactive idea 743 owns that target-width convergence. Pointer, narrow
integer, aggregate, byval, HFA/vector/other ABI expansion, variadic,
function-pointer, and `va_list` receipt also remains blocked. Do not infer any
of these rows from names, mirror spelling, raw operands, or ABI position.

## Non-Goals For The Current Step

- no function-body parameter binding or parameter value IDs
- no `long` or `unsigned long` receipt pending idea 743 target-width policy
- no pointer, narrow, aggregate/byval, HFA/vector/expanded, variadic,
  function-pointer, or `va_list` receipt
- no CFG, block-order, edge, stack-object, hoisted-alloca, local-object, or
  lifetime work
- no ordinary-instruction or terminator expansion beyond landed rows
- no target interpretation, ABI placement, canonicalization, allocation, MIR,
  emission, or legacy-BIR resurrection

## Execution Rules

1. Implement one exact handed-off receiver row per bounded packet.
2. Consume structured logical, ABI-signature, typed-mirror, and flag authority
   directly; do not add a name side table or parallel parameter model.
3. Extend storage/builder/view only if the existing typed function signature
   model lacks an exact destination; wire importer and reachable verification
   in the same packet.
4. Preserve declaration/definition parity, source order, exact type agreement,
   ownership, and whole-module transactional publication.
5. Reject missing, extra, reordered, shape-conflicting, byval, variadic, or
   mirror-conflicting authority with stable diagnostics.
6. Keep all non-authorized parameter and body forms explicitly fail-closed.
7. Reject testcase dispatch, display parsing, expectation weakening,
   supported-to-unsupported changes, and catch-all success.
8. Every code packet requires a fresh build, exact focused proof, neighboring
   malformed proof, production boundary observation, and the
   supervisor-selected regression checkpoint.
9. Keep routine execution updates in `todo.md`; change this runbook only for a
   real route or proof correction.

## Ordered Steps

### Step 4.5.1 - Receive zero, void and target-stable plain scalar signatures

Goal: receive the exact declaration/definition signature surface authorized by
the idea-742 handoff without claiming body parameter identity.

Primary target:

- existing Raw-BIR `FunctionSignature`, `ParameterDef`, function builder/view,
  verifier, and `src/backend/bir/lir_to_bir.cpp` signature import boundary

Actions:

- preserve zero-argument shape only when all three parameter tracks are empty
  and the void-list and variadic flags are false
- preserve explicit void only when `params` contains the single plain
  `TB_VOID` sentinel, both fixed ABI tracks are empty, the void-list flag is
  true, and the variadic flag is false
- for only `int`, `uint`, `long long`, `unsigned long long`, `float`, and
  `double`, require exact three-track count and order, default `TypeSpec` shape,
  `is_byval=false`, nonvariadic state, and exact typed integer/floating mirror
  agreement
- lower those structured types into the existing BIR function signature and
  create ordinary typed `ParameterDef` values by ordinal where the existing
  model requires them; ordinal is BIR storage order, not source identity
- support declarations and definitions whose bodies do not use parameters;
  do not resolve raw body operands or bind them by name/position
- verify exact signature/parameter ownership, type/order, malformed parity,
  duplicates, and whole-module rollback
- keep `long`, `unsigned long`, pointer, narrow, aggregate/byval, expanded,
  variadic, function-pointer, `va_list`, and any non-default shape at
  `UnsupportedFunctionParameters`
- add structural positive, misleading-display, neighboring malformed, and
  transactional tests without matching a focused testcase name

Completion check:

- a fresh build and exact focused proof receive zero/void plus only the six
  target-stable default-shape nonvariadic scalar families named above for
  declarations and definitions; malformed neighbors reject atomically; I686
  `long`/`unsigned long`, body parameter use, and every other excluded shape
  remain fail-closed

### Step 4.5.2 - Reassess remaining function, CFG and local-object authority

Goal: choose the next exact structured function-surface row only after Step
4.5.1 is accepted.

Actions:

- inventory remaining function metadata, block/edge, stack-object,
  hoisted-alloca, local-object, and lifetime rows against actual typed source
  authority
- select one bounded receiver packet where authority is already structured
- create a separate producer-identity initiative for any required raw row;
  never parse labels, names, operands, or rendering
- do not absorb blocked parameter families merely because plain signatures are
  received

Completion check:

- the next packet has one exact typed authority/destination/proof contract, or
  a separate producer initiative records the blocker without scope drift

Inventory result (complete):

- `LirFunction::is_internal` and `can_elide_if_unreferenced` are native
  booleans populated directly from HIR linkage, static, inline, and retained
  helper facts; LIR dead-internal elimination already consumes the elision fact
- current Raw-BIR `FunctionData`, builder, and immutable view have no
  destination for either fact, and the importer silently drops both
- CFG receipt is blocked despite structured block IDs and entry selection:
  branch, conditional-branch, and switch successors remain raw labels, while
  the structured indirect-branch ID form has no production path
- stack slots retain `LirStackSlotId`, owned `TypeSpec`, alignment, and VLA
  state, but production allocation, result, and ownership bindings still cross
  raw `LirOperand`/alloca seams; isolated storage receipt would not unlock a
  production-success path
- body parameter uses remain raw and cannot be bound through signature ordinal,
  display name, or ABI position
- the next exact receiver seam is only the two structured function booleans;
  CFG, local/stack objects, and body binding remain blocked

### Step 4.5.3 - Receive function linkage and elision facts

Goal: preserve the exact structured function linkage/elision metadata already
used by LIR without broadening into CFG, local objects, or body identities.

Primary target:

- Raw-BIR `FunctionData`, function builder/view, reachable verifier, and
  `src/backend/bir/lir_to_bir.cpp` function import/merge boundary

Actions:

- add exactly `is_internal` and `can_elide_if_unreferenced` to the typed
  function container and expose immutable view accessors
- extend builder creation and established duplicate/declaration-definition
  merge paths so neither fact is silently dropped, guessed, or reconstructed
- import both native `LirFunction` booleans for declarations and definitions;
  do not derive them from link names, signature text, function order, body
  presence, or testcase identity
- preserve producer-valid declaration/definition merge behavior and require
  repeated/merged function identities to retain one coherent metadata result;
  reject contradictory or illegal duplicates rather than silently ORing,
  clearing, or replacing facts
- extend reachable Raw and Canonical verification for exact container/view/
  builder agreement, function ownership, duplicate identity, and merge parity
- prove whole-module rollback for malformed or conflicting metadata/merge
  inputs
- add focused structural coverage for every producer-valid boolean
  combination, declaration and definition paths, misleading display/link
  spelling, and neighboring merge conflicts
- add a production-focused retained referenced internal/static or inline/helper
  proof where the LIR elimination policy keeps the function; if elimination
  makes a standalone producer case impossible, use a direct structured LIR
  probe plus a production neighboring observation without weakening the claim
- keep function-body parameter binding, CFG/targets, blocks/edges, stack slots,
  allocas, local objects, lifetime state, and all excluded signature families
  unchanged and fail-closed

Completion check:

- a fresh build and exact focused proof preserve both native booleans through
  Raw and Canonical function views for producer-valid declarations,
  definitions, and merges; malformed/conflicting merges reject atomically; a
  retained production function demonstrates callable receipt where possible;
  and no CFG, local-object, or body-identity surface is admitted

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
  blocked-source disposition; accumulated focused and broader proof passes

### Step 6 - Complete terminators and structured inline-assembly transport

Goal: complete structured terminator receipt and preserve inline-assembly facts
without a parallel value or target-interpretation system.

Actions:

- implement only terminator rows with structured target/value authority
- verify targets, edge arguments/results, successor identity, and malformed CFG
  atomically
- preserve opaque byte-exact assembly text and ordinary operand/result identity
  without parsing, allocation, or target preparation
- keep raw compatibility fail-closed and separate missing producer identity

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

Completing Step 8 makes idea 734 eligible for closure; it does not resume the
parked documentation child automatically. Lifecycle then returns to idea 732
and reruns documentation convergence against the landed implementation from
phase A onward.
