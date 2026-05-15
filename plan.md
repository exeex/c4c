# AArch64 Global Address Materialization Runbook

Status: Active
Source Idea: ideas/open/233_aarch64_global_address_materialization.md

## Purpose

Make AArch64 global, label, and TLS address materialization an explicit
selected-machine-node capability backed by structured relocation/address-kind
facts.

Goal: lower address-producing global/label/TLS cases without hiding them inside
memory load/store support or rendered symbol-name conventions.

Core Rule: preserve address kind and relocation policy as structured target
facts before printing; do not infer GOT, direct, label, or TLS behavior from
text.

## Read First

- `ideas/open/233_aarch64_global_address_materialization.md`
- Existing BIR/global value handling and prepared value-home records.
- Current AArch64 instruction record and printer support for symbol-backed
  operands.
- Any diagnostics that currently defer global, label, or TLS address
  materialization.

## Current Targets

- Structured carriers for direct global addresses, label addresses, GOT-backed
  globals, and TLS-relative globals.
- AArch64 selected machine nodes that materialize those addresses into prepared
  result homes.
- Printer support for explicit relocation operands once machine-node facts are
  present.
- Focused backend tests that prove semantic address lowering, not
  name-shaped shortcuts.

## Non-Goals

- Do not rebuild the archived `globals.cpp` implicit `x0` scratch convention.
- Do not infer GOT or TLS policy from rendered symbol names.
- Do not claim global memory load/store lowering as address materialization.
- Do not broaden into ordinary memory load/store lowering, dynamic stack/frame
  setup, or unrelated call lowering.

## Working Model

- Prepared value homes and allocation results decide where the address result
  lives.
- A structured address-kind carrier decides whether the selected node is direct
  page+low12, GOT load, label address, or TLS-relative.
- AArch64 printing consumes selected machine-node records; it does not recover
  relocation semantics from textual assembly.
- Unsupported or incomplete address states should produce explicit diagnostics
  rather than fabricated assembly.

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

Goal: identify the current carriers, deferrals, and printer records involved in
global, label, and TLS address-producing cases.

Primary Target: shared BIR/prepared value facts and AArch64 instruction/printer
records related to global symbols, labels, string constants, and TLS globals.

Actions:

- Inspect existing symbol-backed memory operands and value-home preparation.
- Locate current diagnostics or unsupported paths for address materialization.
- Identify where relocation/address-kind policy is currently represented or
  missing.
- Record the first implementation packet target in `todo.md` before code work.

Completion Check: the executor can name the exact carrier or dispatch gap that
Step 2 will address, without changing source intent.

### Step 2: Add Structured Address-Kind Carriers

Goal: make direct global, label, GOT-backed global, and TLS-relative address
materialization explicit before AArch64 selection.

Primary Target: the smallest shared/prepared layer that already owns global
address policy or value-home preparation.

Actions:

- Add carrier fields or records for address kind, symbol/label identity, TLS
  mode, and relocation operands.
- Preserve prepared result-home authority and reserved scratch policy.
- Emit explicit diagnostics when a required address-kind fact is unavailable.
- Keep memory-access carriers separate from address-producing carriers.

Completion Check: dumps or targeted observations expose structured address
materialization facts independently from load/store memory operands.

### Step 3: Select AArch64 Address Materialization Nodes

Goal: lower supported address-producing facts into AArch64 selected machine
nodes.

Primary Target: AArch64 block/operation dispatch and machine-instruction record
types.

Actions:

- Add selected node variants for direct page+low12 materialization, label
  addresses, GOT loads, and TLS-relative address materialization.
- Consume prepared result homes instead of writing through an implicit scratch
  register.
- Preserve relocation operands, address kind, and result register facts in the
  selected node.
- Leave unsupported address kinds deferred with explicit diagnostics.

Completion Check: selected-node dumps or focused backend tests show structured
address nodes for at least one direct global/label path and one policy-specific
path when the required facts exist.

### Step 4: Print Relocation-Aware AArch64 Sequences

Goal: print valid AArch64 assembly from selected address-materialization nodes.

Primary Target: AArch64 terminal printer paths for selected machine records.

Actions:

- Print direct global and label materialization using page and low-12
  relocation steps from structured operands.
- Print GOT-required globals as explicit GOT loads from structured policy.
- Print TLS materialization only when thread-pointer-relative facts are
  available.
- Avoid fallback text templates that infer policy from symbol spelling.

Completion Check: printer output is driven by machine-node fields and fails
explicitly when required relocation operands are absent.

### Step 5: Validate Semantic Coverage

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
