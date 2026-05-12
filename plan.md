# Aggregate ABI Classification Structured Facts Runbook

Status: Active
Source Idea: ideas/open/174_aggregate_abi_classification_structured_facts.md

## Purpose

Make one aggregate ABI classification boundary consume durable structured
layout and ABI facts instead of treating rendered type or signature strings as
semantic authority.

Goal: land a bounded aggregate ABI classification improvement where byval,
sret, HFA/direct, memory, stack, or return decisions are traceable to
structured facts or to an explicit legacy fallback.

## Core Rule

Do not add new rendered-signature, rendered-type, or instruction-text parsing
as the authority for aggregate ABI classification. Metadata-rich aggregate
inputs must use structured layout and ABI facts, or fail/report a clear
metadata gap.

## Read First

- Source idea: `ideas/open/174_aggregate_abi_classification_structured_facts.md`
- Recent closed audit context:
  `ideas/closed/173_aggregate_layout_identity_structured_boundary.md`
- Candidate implementation surfaces named by the source idea:
  - BIR aggregate call classification
  - AArch64 direct LIR aggregate route
  - `CallArgAbiInfo` / `CallResultAbiInfo`
  - byval/sret flags, size, alignment, and aggregate-kind facts

## Current Scope

- Choose one bounded aggregate ABI classification boundary.
- Prefer the path with the clearest structured facts already available.
- Preserve final ABI spelling and LLVM text as output surfaces.
- Keep legacy/no-metadata behavior explicit and compatibility-labeled.
- Prove the selected route with build proof plus focused aggregate ABI tests.

## Non-Goals

- Do not retire every AArch64 direct LIR text parser in one pass.
- Do not replace ABI printers or final LLVM textual syntax.
- Do not reclassify unrelated scalar call ABI behavior.
- Do not weaken backend expectations, mark supported paths unsupported, or
  rewrite tests to match current spelling-derived behavior.
- Do not broaden into unrelated type identity cleanup.

## Working Model

- Structured facts, when present, are the first authority for the selected
  aggregate ABI decision.
- Rendered strings may remain display, printing, diagnostics, or final-output
  payloads.
- Legacy inputs without structured metadata may continue through a named
  compatibility fallback.
- Metadata-rich inputs with missing or mismatched structured facts must not
  silently take the old spelling-derived path.

## Execution Rules

- Keep each implementation packet focused on one classification boundary.
- Prefer semantic lowering/generalization over named testcase fixes.
- Preserve existing aggregate ABI tests unless a stronger contract replaces
  them.
- Make mismatch or missing-metadata behavior observable through code shape,
  diagnostics, verifier behavior, or focused tests.
- For code-changing steps, run a fresh build or compile proof before claiming
  the step complete. Use targeted backend CTest coverage for the selected
  route, then escalate to broader validation if the touched path crosses more
  than one backend bucket.

## Ordered Steps

### Step 1: Select the Aggregate ABI Boundary

Goal: identify the narrowest aggregate ABI classification path that can be
moved to structured facts first.

Primary target: either BIR aggregate call classification or the AArch64 direct
LIR aggregate route.

Actions:
- Inspect the aggregate parameter and return classification flow for byval,
  sret, HFA/direct, memory, stack, and return decisions.
- Identify where rendered type, rendered signature, or instruction text is
  still used as first authority.
- Identify which structured facts are already present at that boundary:
  layout size, alignment, aggregate kind, `CallArgAbiInfo`,
  `CallResultAbiInfo`, byval/sret metadata, or equivalent records.
- Choose one path where available structured metadata can replace or guard the
  spelling-derived decision without a broad backend rewrite.

Completion check:
- The selected boundary is named in `todo.md` with the files/functions to
  change, the structured facts available there, and the focused proof command
  the supervisor should delegate to the executor.

### Step 2: Route Metadata-Rich Inputs Through Structured Facts

Goal: make the selected boundary classify metadata-rich aggregate inputs from
structured layout and ABI metadata.

Primary target: the files/functions selected in Step 1.

Actions:
- Thread or expose the structured facts needed by the selected classifier.
- Replace the chosen spelling-derived decision with structured fact lookup
  when metadata is present.
- Preserve output spelling as the final printer surface, not as the semantic
  source for the decision.
- Keep the change behavior-preserving for unrelated scalar ABI paths and
  untouched aggregate routes.

Completion check:
- The selected byval, sret, HFA/direct, memory, stack, or return decision uses
  structured facts when those facts are present, and unrelated scalar ABI
  classification remains unchanged.

### Step 3: Make Legacy and Mismatch Behavior Explicit

Goal: prevent structured metadata gaps from being hidden by the old text-based
classification route.

Primary target: the compatibility or validation branch adjacent to the Step 2
  classifier.

Actions:
- Name the legacy/no-metadata fallback as compatibility behavior.
- Add a fail-closed, diagnostic, verifier, assertion, or explicit unsupported
  path for generated metadata-rich inputs with missing or inconsistent
  structured facts, as appropriate for the selected boundary.
- Avoid synthesizing structured facts from rendered names at the comparison or
  classification site.

Completion check:
- A metadata-rich aggregate cannot silently fall through to spelling-derived
  classification when required structured facts are missing or mismatched.

### Step 4: Add Focused Aggregate ABI Coverage

Goal: prove the new authority boundary with tests that would catch a
spelling-only classification collision.

Primary target: focused backend, frontend-to-backend, or route tests for the
selected aggregate ABI path.

Actions:
- Add or adjust tests for the selected aggregate parameter/return, byval/sret,
  HFA/direct, memory, or stack route.
- Include at least one case where equal or familiar rendered spelling would be
  insufficient to prove the structured classification contract.
- Keep expectations strong; do not downgrade supported behavior or weaken
  existing aggregate ABI checks.
- Prefer nearby coverage around the selected boundary over broad snapshot
  churn.

Completion check:
- Targeted tests fail without the structured-fact classification change and
  pass with it, while existing aggregate ABI contracts remain at least as
  strong.

### Step 5: Validate and Summarize the Slice

Goal: leave the lifecycle state ready for supervisor review and commit.

Actions:
- Run the delegated build or compile proof.
- Run the focused backend CTest or route coverage for the selected aggregate
  ABI path.
- If the implementation touched multiple backend buckets, recommend broader
  validation such as `ctest --test-dir build -j --output-on-failure` or the
  repo-native regression guard.
- Update `todo.md` with the selected boundary, proof commands, results, and
  any remaining compatibility fallback.

Completion check:
- `todo.md` records the latest packet status, proof result, and any follow-up
  notes. The source idea acceptance criteria can be evaluated from code,
  tests, and proof logs without relying on chat history.
