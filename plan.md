# LIR Nominal Vector Store and Vector-Schema Migration Runbook

Status: Active
Source Idea: ideas/open/840_lir_nominal_vector_store_schema_migration.md

## Purpose

Make `LirVectorRef` and its store the sole owner of vector lane count and typed
element-family facts, then migrate bounded vector schemas to consume those
facts without retaining row-local shape authority.

## Goal

Replace vector shape/type-string mirrors with native vector-store facts while
preserving accepted 754, 811, 814, and 815 behavior and keeping aggregate
element handling behind the already accepted 838 aggregate-store dependency.

## Core Rule

Do not infer vector element identity, lane count, masks, or operation shape
from rendered text, testcase names, or row-local mirrors. Each migrated vector
consumer must read an authoritative vector-store fact and fail closed for
malformed, missing, wrong-family, or incoherent data.

## Read First

- `ideas/open/840_lir_nominal_vector_store_schema_migration.md`
- `ideas/closed/838_lir_canonical_module_owned_aggregate_ref_store_convergence.md`
- Closed evidence for preserved vector routes: 754, 811, 814, and 815
- Current LIR vector schema, verifier, printer, and lowering surfaces before
  editing

## Current Targets And Scope

- Add or complete the nominal `LirVectorRef` / vector-store representation for
  lane count and typed element-family references.
- Migrate InsertElement, ExtractElement, ShuffleVector, mask handling, poison,
  and second-shape consumers only where they can consume the store-backed
  vector facts.
- Preserve aggregate elements through the accepted 838 aggregate ref/store
  model when an element family is aggregate.
- Remove `LirNativeVectorShape`, mask text, and operation type-string
  duplicates only after every named vector schema reads the store with
  equivalent malformed checks.

## Non-Goals

- Do not reopen accepted 754, 811, 814, or 815 capability.
- Do not change aggregate identity ownership beyond consuming accepted 838
  aggregate refs for aggregate elements.
- Do not perform scalar, function-signature, restricted-union, direct-HIR,
  family-overloaded verifier/printer, collector, global/extern, Raw-BIR final
  convergence, or universal deletion work.
- Do not weaken fail-closed verifier behavior or add testcase-shaped branches.

## Working Model

Vector facts must have one semantic source: a module-owned nominal vector
record that carries lane count and the typed element family. Operation schemas
may keep compatibility fields only while a named consumer is not migrated, and
only as check-only legacy data. Once a named operation and its verifier/printer
path consume `LirVectorRef`, the duplicate shape or text authority for that
same path should be removed or made non-authoritative.

## Execution Rules

- Keep each implementation packet bounded to one producer/consumer family or
  one deletion gate.
- Add nearby positive and malformed coverage with each migration packet.
- Preserve wrong-family rejection: scalar, aggregate, function, and vector refs
  must not be implicitly interchangeable.
- Do not delete legacy fields until the migrated named consumers have parity
  coverage and malformed checks.
- For code-changing steps, run a fresh build plus focused vector/lowering/
  verifier/printer proof. Escalate to broader CTest when shared schema,
  verifier, printer, or dispatcher surfaces accumulate several packets or the
  blast radius grows.

## Steps

### Step 1 - Trace Current Vector Authority And Pick The First Store Seam

Goal: identify the existing vector shape/type authority, every duplicate
mirror, and the smallest first vector-store seam to migrate.

Primary target: current LIR vector schema, lowering, verifier, and printer
definitions.

Actions:

- Inspect vector-related LIR structs, builders, verifier helpers, printers, and
  tests for `LirNativeVectorShape`, masks, lane count, element type, and
  operation type strings.
- Classify each vector consumer as producer, verifier, printer, receiver, or
  compatibility-only.
- Select the first bounded vector-store seam that can carry lane count and
  typed element family without text recovery.
- Record any aggregate-element dependency as consumption of accepted 838 facts,
  not a new aggregate-owner route.

Completion check:

- The executor can name the first store-backed vector fact, the first migrated
  consumer, the duplicate fields that remain compatibility-only, and the
  focused proof that will demonstrate the seam.

### Step 2 - Introduce The Nominal Vector Store Fact

Goal: add the bounded `LirVectorRef` / store representation and populate it for
the selected first vector producer.

Primary target: vector schema and construction surfaces selected in Step 1.

Actions:

- Add or complete the vector-store entry for lane count and typed element
  family.
- Populate it from native facts at the selected producer seam.
- Validate missing, malformed, wrong-family, and aggregate-element coherence
  through native checks.
- Keep legacy vector shape/text fields only as compatibility data until named
  consumers migrate.

Completion check:

- Fresh build and focused tests prove the selected producer creates a native
  vector-store fact and rejects malformed or incoherent facts without using
  text-derived element identity.

### Step 3 - Migrate Bounded Vector Operation Consumers

Goal: move one named vector operation family at a time to consume
`LirVectorRef` as its semantic shape/type authority.

Primary target: InsertElement, ExtractElement, ShuffleVector, mask, poison, and
second-shape consumers in bounded packets.

Actions:

- For each selected operation family, replace row-local shape/type-string
  authority with vector-store lookup and typed element checks.
- Preserve accepted behavior from 754, 811, 814, and 815 without reopening
  their source scope.
- Add valid and malformed tests for lane count, element-family mismatch,
  wrong-family refs, masks, and second-shape coherence as applicable.
- Keep each packet small enough that the exact changed operation family is
  visible in proof.

Completion check:

- Every migrated operation family has focused positive and negative coverage,
  verifier/printer behavior remains equivalent where required, and nonselected
  vector consumers stay fail closed or compatibility-only.

### Step 4 - Retire Duplicate Vector Mirrors For Migrated Consumers

Goal: remove stale vector shape/text authority only after named migrated
consumers no longer need it.

Primary target: `LirNativeVectorShape`, mask text, and operation type-string
  duplicates tied to fully migrated consumers.

Actions:

- Delete or demote duplicate fields only when all named consumers for that
  field read the vector store with equivalent malformed checks.
- Update verifier and printer paths so rendering is one-way output, not
  semantic input.
- Preserve any legacy compatibility boundary that still has an unmigrated
  named consumer.

Completion check:

- Focused vector proof plus a fresh build show no migrated consumer relies on
  row-local shape mirrors, mask text, or operation type strings for semantic
  vector facts.

### Step 5 - Prove The Vector Migration And Return The Queue

Goal: establish bounded completion for the 840 vector route and leave later
queue owners unchanged.

Primary target: vector lowering, verifier, printer, and operation tests.

Actions:

- Run a fresh build.
- Run focused vector lowering/verifier/printer proof and nearby operation
  coverage for all migrated families.
- Run broader CTest if shared verifier/printer/schema changes or accumulated
  packets justify it.
- Confirm `841`, `842`, `843`, `845`, `846`, `847`, `813`, and the parked
  `829/830/831/836` return chain remain separate owners.

Completion check:

- The supervisor has accepted fresh build and vector-focused proof with no
  expectation downgrades, and the completion record can state exactly which
  vector mirrors were retired and which compatibility boundaries remain.
