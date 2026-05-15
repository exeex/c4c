# AArch64 Global Address Materialization Runbook

Status: Active
Source Idea: ideas/open/233_aarch64_global_address_materialization.md
Resumed After: ideas/closed/247_explicit_got_materialization_policy.md

## Purpose

Make AArch64 global, label, GOT-backed global, and TLS address materialization
an explicit selected-machine-node capability backed by structured relocation
and address-kind facts.

Goal: finish address-producing global/label/TLS cases without hiding them
inside memory load/store support or rendered symbol-name conventions.

Core Rule: preserve address kind and relocation policy as structured target
facts before printing; do not infer GOT, direct, label, or TLS behavior from
text, extern status alone, or downstream assembler relocation names.

## Read First

- `ideas/open/233_aarch64_global_address_materialization.md`
- `ideas/closed/247_explicit_got_materialization_policy.md`
- Current prepared address-materialization carriers and AArch64 selected
  `AddressMaterializationRecord` support.
- Current AArch64 printer diagnostics for label, GOT, and TLS records.

## Current Targets

- Completed direct page+low12 carriers, selected records, and printer output
  for direct globals and string constants.
- Completed label address selection with structured label page+low12 facts.
- Completed explicit GOT policy through prepared facts and AArch64 selected
  `GotPageLow12` records.
- Remaining TLS materialization facts.
- Remaining terminal printer support for label, GOT, and TLS records once their
  selected records carry every required relocation operand.
- Focused backend tests that prove semantic address lowering, not name-shaped
  shortcuts.

## Non-Goals

- Do not rebuild the archived `globals.cpp` implicit `x0` scratch convention.
- Do not infer GOT or TLS policy from rendered symbol names.
- Do not claim global memory load/store lowering as address materialization.
- Do not broaden into ordinary memory load/store lowering, dynamic stack/frame
  setup, or unrelated call lowering.

## Working Model

- Prepared value homes and allocation results decide where the address result
  lives.
- Structured carriers decide whether the selected node is direct page+low12,
  label page+low12, GOT page+low12, or TLS-relative.
- AArch64 printing consumes selected machine-node records; it does not recover
  relocation semantics from textual assembly.
- Unsupported or incomplete address states should produce explicit diagnostics
  rather than fabricated assembly.
- Direct page+low12 global/string-constant printing, label selected records,
  and GOT selected records are completed milestones. Reopen them only for
  shared coherence needed by TLS or terminal printer work.

## Execution Rules

- Keep each code slice narrow enough to prove with build plus targeted backend
  checks.
- Add printer output only after the machine node carries every relocation and
  operand fact the printer needs.
- Prefer semantic lowering paths that cover nearby same-feature cases.
- Treat expectation rewrites, named symbol shortcuts, or single-fixture matches
  as route drift unless they accompany real carrier/lowering support.

## Ordered Steps

### Step 1: Inspect Existing Global Address Surfaces

Status: Completed.

Goal: identify the current carriers, deferrals, and printer records involved in
global, label, and TLS address-producing cases.

Completion Check: the exact carrier and dispatch gaps were identified without
changing source intent.

### Step 2: Add Structured Address-Kind Carriers

Status: Completed.

Goal: make direct global, label, GOT-backed global, and TLS-relative address
materialization explicit before AArch64 selection.

Completion Check: prepared observations expose structured address
materialization facts independently from load/store memory operands.

### Step 3: Select AArch64 Address Materialization Nodes

Status: Completed for direct globals, string constants, labels, and GOT-backed
globals.

Goal: lower supported address-producing facts into AArch64 selected machine
nodes.

Completion Check: focused backend tests show structured address records for
direct, label, and GOT-backed paths when required facts exist.

### Step 4: Print Direct Page+Low12 AArch64 Sequences

Status: Completed by commit `90640a317`.

Goal: print valid AArch64 assembly for selected direct global and string
constant address-materialization nodes.

Completion Check: direct global and string constant printer tests pass through
structured record fields, not symbol-name inference.

### Step 5: Populate Label Address Materialization

Status: Completed by commit `d266b1816`.

Goal: make label address materialization reach selected AArch64 records with
all relocation operands required for terminal printing.

Completion Check: focused tests show label address materialization selected
with structured label/page/low12 facts.

### Step 6: Populate GOT-Backed Global Materialization

Status: Completed through prerequisite idea
`ideas/closed/247_explicit_got_materialization_policy.md`, accepted through
commit `8915656eb`.

Goal: make GOT-required global address materialization explicit before terminal
printing.

Completion Check: explicit GOT policy reaches prepared facts and selected
AArch64 `GotPageLow12` records without inferring from symbol spelling or
`is_extern` alone.

### Step 7: Specify TLS Materialization Facts

Goal: define and carry the TLS facts required before selected TLS records can
print terminal AArch64 assembly.

Primary Target: prepared TLS address-kind carrier, selected TLS record fields,
and fail-closed printer diagnostics.

Actions:

- Identify the required thread-pointer-relative relocation facts for the
  supported TLS model.
- Keep TLS model selection explicit; do not infer TLS behavior from symbol
  names or storage class text.
- Ensure selected TLS records carry result-home authority, symbol identity,
  TLS model, and thread-pointer-relative operands.
- Leave terminal printing fail-closed until the record contains every field the
  printer needs.

Completion Check: selected TLS records expose structured TLS facts, and missing
facts produce diagnostics that name the absent TLS policy or relocation input.

### Step 8: Print Remaining Relocation-Aware AArch64 Sequences

Goal: print valid AArch64 assembly for label, GOT, and TLS address-
materialization records once their selected records are complete.

Primary Target: AArch64 terminal printer paths for selected label, GOT, and TLS
machine records.

Actions:

- Print label page+low12 materialization from structured label operands.
- Print GOT-required globals as explicit GOT loads from structured relocation
  fields.
- Print TLS materialization only after Step 7 supplies the required
  thread-pointer-relative facts.
- Keep deferred printer diagnostics for any selected record missing required
  relocation fields.

Completion Check: printer output for label, GOT, and TLS is driven by selected
record fields and fails explicitly when required relocation operands are
absent.

### Step 9: Validate Semantic Coverage

Goal: prove the feature across nearby cases without overfitting one fixture.

Primary Target: focused AArch64 backend tests plus the supervisor-selected
broader check.

Actions:

- Add or update tests for direct global address, label address, GOT-required
  global, and TLS global paths as implementation support lands.
- Verify global load/store cases remain separate from address materialization.
- Run build proof and the delegated narrow backend test subset after each code
  slice.
- Escalate to broader validation once multiple address kinds or printer paths
  are active.

Completion Check: direct, GOT, label, and TLS proof cases either pass through
structured lowering or fail with explicit unsupported diagnostics; no supported
case depends on name-shaped matching.
