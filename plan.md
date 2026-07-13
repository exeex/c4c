# LIR-To-New-BIR Container And Import Completeness Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md
Activated from: user-directed interruption of phase-C documentation convergence

## Purpose

Implement the target-independent new Raw-BIR typed storage and the complete
LIR-to-new-BIR import boundary so every semantic fact already carried by the
existing typed LIR can be received, verified and published without loss.

## Goal

Replace the current bounded importer with an exhaustive, explicit and
module-transactional path from the unchanged typed LIR surface into verified
new Raw BIR. New Raw-BIR schema is the destination authority; typed LIR is the
input authority. Quarantined legacy-BIR structures are evidence and
disposition history only and must not be revived as the new schema.

## Core Rule

Every current typed LIR fact must have one typed new Raw-BIR destination, one
explicit importer disposition and one reachable verifier rule. Valid current
LIR may not remain behind an unsupported fallback. Import either publishes one
complete verified module or publishes nothing.

This runbook owns C++ containers, builders, views, verifier, importer, build
wiring, focused tests and nearby same-feature coverage. It does not own
canonicalization, target preparation, allocation, MIR or emission. LIR remains
unchanged except for the source idea's sole evidence-gated minimal inline-asm
constraint-carrier exception.

## Read First

- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- existing typed LIR declarations and their producers as source authority
- current new-BIR core/schema, builders, views, verifier and importer owners
- `src/backend/bir/lir_to_bir/README.md` and
  `src/backend/bir/LEGACY_COVERAGE.md` as coverage/disposition evidence
- `ideas/closed/735_bir_phase_a_import_raw_document_convergence.md` as
  pre-implementation historical contract evidence, not a substitute for code

## Execution Rules

1. Inventory actual typed LIR declarations and every new-BIR/import/verifier
   owner before changing schema. Resolve each row as already covered, missing
   typed destination, missing importer wiring or stale documentation.
2. Use stable typed identities and structured fields. Never reconstruct
   semantics from rendered text, names, vector position or legacy layout.
3. Extend new Raw-BIR storage, builders, immutable views, verifier and importer
   together in coherent semantic families. Do not add storage without a
   producer, verifier and focused proof.
4. Preserve ordinary SSA/value, type, block/edge, object, symbol, initializer,
   effect and source-order identity. Inline assembly uses ordinary operands and
   results and keeps assembly text opaque and byte-exact.
5. Keep import module-transactional. A rejected row, invalid reference,
   duplicate identity, malformed CFG or verifier failure publishes no partial
   graph, capability, fixup table or mixed state.
6. Reject legacy-schema resurrection, string fixups, catch-all success,
   testcase-shaped dispatch, expectation weakening and unsupported downgrade.
7. Every code packet requires a fresh build or compile plus the supervisor's
   exact narrow proof and nearby same-feature coverage. Use accumulated broader
   checkpoints after shared schema/importer slices and at the final gate.
8. Update routine execution state in `todo.md`. Change this runbook or source
   only for a real route, scope or proof correction. Canonical root regression
   logs remain supervisor-owned.

## Ordered Steps

### Step 1 - Establish the exhaustive coverage ledger and schema/import boundary

Goal: derive the implementation packet map from actual typed LIR and establish
the exact new Raw-BIR destination and import/verifier owner for every family.

Actions:

- enumerate every `LirInst` and `LirTerminator` alternative plus module,
  function, parameter/result, block/edge, value/type, global/string/extern,
  symbol/link, initializer, specialization, stack-object/alloca,
  intrinsic-requirement and other metadata fields
- inspect current new-BIR storage/builders/views/verifier/importer and classify
  every row as covered, destination-missing, wiring-missing or stale-doc
- use legacy-BIR coverage only to ensure no historical semantic family is
  accidentally omitted; do not copy or compile quarantined structs as schema
- define stable identity, ownership, optional/error form, source ordering,
  destination field, builder operation, view exposure, verifier obligation,
  importer rule and positive/negative proof for every row
- identify dependency order and bounded family packets without implementing a
  later backend phase or changing typed LIR

Completion check:

- one checked, exhaustive implementation ledger has no catch-all row and gives
  every current typed LIR fact an exact typed Raw-BIR/import/verifier/proof
  disposition; the first coherent C++ family packet is unambiguous

### Step 2 - Complete foundational identities, types and module containers

Goal: make new Raw BIR hold the stable module-level identities and structured
type/value foundations needed by all later family imports.

Actions:

- implement the required typed storage, IDs, builders and immutable views for
  the foundational type/value/module families identified by Step 1
- add verifier rules for ownership, uniqueness, reference domain, ordering and
  malformed/duplicate cases
- wire the corresponding importer rows without string or positional recovery
- add neighboring positive and negative coverage

Completion check:

- the family compiles in a fresh build, its exact delegated narrow proof is
  green, and malformed foundational state cannot publish

### Step 3 - Complete globals, strings, externs, symbols and initializers

Goal: losslessly receive all module object and declaration semantics, including
initializer topology and link/symbol identity.

Actions:

- implement the required containers/builders/views for globals, string-pool
  state, extern declarations/indexes, symbols/linkage and typed initializers
- import every actual LIR variant explicitly and verify cross-references,
  topology, ordering, uniqueness and optional/error forms
- add nearby positive/negative proof for each meaningful variant

Completion check:

- fresh build and narrow proof pass; an accumulated module-level checkpoint
  proves Steps 2-3 together and no module family remains generically unsupported

### Step 4 - Complete functions, signatures, CFG and local objects

Goal: receive complete function structure rather than the current restricted
zero-parameter, void-return, first-entry-block subset.

Actions:

- implement typed parameter/result/signature, block/edge, stack-object,
  hoisted-alloca and function metadata storage plus builders/views
- import arbitrary valid block order and preserve entry identity, CFG edge
  identity, source ordering and local object ownership
- verify references, signatures, CFG shape, local lifetimes/ownership and all
  malformed or duplicate cases covered by the source surface

Completion check:

- fresh build and exact narrow proof pass with neighboring function/CFG/object
  variants; failure remains module-transactional

### Step 5 - Complete ordinary instruction semantic families

Goal: give every current ordinary `LirInst` alternative a typed new-BIR node,
ordinary operand/result wiring and verifier coverage.

Actions:

- implement coherent instruction-family packets in dependency order rather
  than a named-test switch
- preserve opcode, typed operands/results, effects, object/symbol references,
  metadata and source order directly from typed LIR
- extend builder/view/verifier/importer and nearby positive/negative tests for
  each family; no valid alternative may fall through unsupported

Completion check:

- every current instruction variant is explicitly dispatched, fresh build and
  family proofs pass, and an accumulated Steps 2-5 checkpoint is green

### Step 6 - Complete terminators and structured inline-assembly transport

Goal: complete CFG terminator receipt and preserve inline-assembly semantics
without creating a parallel value or target-interpretation system.

Actions:

- implement and import return forms, conditional/unconditional branches,
  switch, indirect branch, unreachable and every other actual terminator
- verify targets, edge arguments/results, successor identity and malformed CFG
  atomically
- preserve byte-exact opaque assembly text, ordinary operand/result identities
  and evidenced constraint requirements against their ordinary positions and
  roles; use the source idea's LIR-carrier exception only if current typed
  fields are concretely proven insufficient

Completion check:

- all current terminators and inline-assembly variants have neighboring
  positive/negative proof, fresh build and narrow proof pass, and no parsing,
  allocation or target preparation has entered Raw BIR

### Step 7 - Integrate the complete dispatcher, verifier and build boundary

Goal: prove all semantic-family slices form one production importer and one
reachable Raw-BIR verification/publication boundary.

Actions:

- remove obsolete bounded-surface rejections only where typed receiving,
  dispatch and verifier coverage now exists
- integrate all migrated importer translation units exactly once in the build
  and eliminate duplicate or quarantined legacy authority
- prove deterministic diagnostics, reference resolution and whole-module
  rollback across failures occurring early, mid-module and at final verify
- reconcile the coverage ledger and implementation-status documentation with
  the callable production path

Completion check:

- a fresh build plus accumulated broader proof passes across module, function,
  instruction and terminator families; no valid current-LIR row is unsupported
  and no failure exposes partial state

### Step 8 - Prove lossless completeness and transactional publication

Goal: close 734 only on exhaustive structured coverage and end-to-end atomicity,
not on a sample testcase or documentation assertion.

Actions:

- re-enumerate the actual typed LIR variants and metadata fields against the
  final checked ledger, destinations, dispatcher, verifier and tests
- prove representative round-trip/structural parity for every meaningful
  variant without using rendering as semantic authority
- run the supervisor-selected fresh build, focused suite and broader/full
  regression gate appropriate to the shared schema/importer changes
- audit diff scope, fail-closed diagnostics, no partial publication, no legacy
  schema revival, no forbidden downstream work and no weakened expectations

Completion check:

- every acceptance criterion in idea 734 is evidenced by callable code and
  tests, the final exhaustive ledger has no missing or unsupported valid row,
  and the supervisor can close 734 as one validated implementation initiative

## Runbook Completion And Handoff

Completing Step 8 makes idea 734 eligible for closure; it does not resume Child
C. After 734 is accepted and closed, lifecycle returns to idea 732 and reruns
documentation convergence from phase A, then B, then C and onward against the
landed implementation. The earlier phase-A/B acceptance and phase-C C1-C6
slices remain historical evidence, but none may be treated as the current
post-734 acceptance gate without re-execution and revalidation.
