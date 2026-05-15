# Explicit GOT Materialization Policy Runbook

Status: Active
Source Idea: ideas/open/247_explicit_got_materialization_policy.md
Supersedes: parked Step 6 of ideas/open/233_aarch64_global_address_materialization.md

## Purpose

Create the explicit policy source required before AArch64 GOT-backed global
address materialization can be selected.

Goal: let prepared address materialization classify a global address as
GOT-required from structured compiler policy, not from symbol text or extern
status alone.

Core Rule: GOT materialization policy must be explicit and source-owned before
AArch64 selection consumes it. Do not infer GOT from symbol spelling,
`is_extern`, fixture names, or downstream assembler relocation enums.

## Read First

- `ideas/open/247_explicit_got_materialization_policy.md`
- `ideas/open/233_aarch64_global_address_materialization.md`
- Current BIR global metadata and CLI target option handling.
- Prepared address materialization carriers and AArch64 selection diagnostics.
- Commit `236be6f41`, which records the blocker.

## Current Targets

- A narrow policy source that can distinguish direct page+low12 globals from
  GOT-required globals.
- Prepared address materialization facts that publish `GotGlobal` only when
  that explicit policy exists.
- Diagnostics for missing or unsupported GOT policy.
- Focused proof that policy reaches prepared/MIR selection without name-shaped
  shortcuts.

## Non-Goals

- Do not implement terminal GOT assembly printing in this prerequisite unless a
  later packet explicitly delegates it after policy exists.
- Do not build a full relocation-model or platform ABI matrix beyond the
  minimum needed to unblock GOT address materialization.
- Do not change direct global, string constant, or label address behavior except
  for shared carrier compatibility.
- Do not continue Step 6 of idea 233 by inferring GOT from `is_extern`.

## Working Model

- Frontend/target/global metadata must provide a structured policy bit or
  equivalent decision that a global address requires GOT materialization.
- Prepared address materialization copies that policy into its own records.
- AArch64 selection may select `GotGlobal` only when result-home authority and
  explicit GOT policy are both present.
- Unsupported or absent policy states should fail before terminal printing with
  diagnostics that identify the missing policy input.

## Execution Rules

- Keep each packet narrow: policy source first, prepared publication second,
  AArch64 consumption third.
- Add tests at the layer being changed; do not prove policy by changing only
  printer expectations.
- Preserve direct and label address-materialization tests while adding GOT
  policy support.
- Stop and report if the only available policy would be symbol spelling or
  `is_extern` alone.

## Ordered Steps

### Step 1: Locate The Policy Owner

Goal: choose the narrowest existing compiler layer that should own explicit
GOT-required policy.

Primary Target: TargetProfile, CLI option transfer, BIR globals, and prepared
address materialization inputs.

Actions:

- Inspect where target relocation options or global linkage/preemptibility
  facts could be represented without text inference.
- Decide whether the first policy should be target-wide, per-global, or a
  small structured combination of both.
- Record any unsupported policy dimensions as diagnostics, not implicit
  defaults.
- Update `todo.md` with the chosen implementation packet and proof command.

Completion Check: the next executor can name the exact owner and field(s) to
add before touching AArch64 selection.

### Step 2: Publish Explicit GOT Policy Into Prepared Facts

Goal: make prepared address materialization observe and dump explicit GOT
policy.

Primary Target: BIR/prepared global-address carrier code and prepared dump or
focused tests.

Actions:

- Add the selected policy field(s) to the owner from Step 1.
- Carry the policy into prepared address materialization records.
- Publish `PreparedAddressMaterializationKind::GotGlobal` only from explicit
  policy.
- Emit missing-policy diagnostics when a GOT-required test cannot be classified
  from structured facts.

Completion Check: focused prepared tests or dumps show direct and GOT-required
globals distinguished by structured policy, not by symbol spelling or
`is_extern` alone.

### Step 3: Consume GOT Policy In AArch64 Selection

Goal: let AArch64 selection select GOT-backed address materialization records
when prepared policy is complete.

Primary Target: AArch64 address-materialization dispatch and selected record
construction.

Actions:

- Consume the prepared `GotGlobal` kind and relocation identity facts.
- Preserve result-home authority and symbol identity in the selected record.
- Keep GOT selection deferred or missing-fact when policy or relocation inputs
  are incomplete.
- Do not add terminal GOT printing unless the record already carries every
  required field and the supervisor delegates that packet.

Completion Check: selected-record tests prove GOT-backed globals reach a
structured `GotGlobal` record, while direct globals still select direct
page+low12 records.

### Step 4: Validate And Hand Back To Address Materialization

Goal: prove the prerequisite and make idea 233 resumable at GOT Step 6.

Primary Target: focused backend tests plus the supervisor-selected validation
subset.

Actions:

- Run the delegated build and focused tests for policy, prepared carriers, and
  AArch64 selection.
- Record proof and remaining handoff notes in `todo.md`.
- Do not close idea 233; report that its Step 6 can resume once this
  prerequisite is accepted.

Completion Check: explicit GOT policy exists end to end through prepared and
selected records, and no accepted proof depends on forbidden GOT inference.
