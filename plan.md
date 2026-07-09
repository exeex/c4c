# String-Constant Local-Memory Policy Runbook

Status: Active
Source Idea: ideas/open/630_string_constant_local_memory_policy.md

## Purpose

Define the RV64 local-memory policy for selected bases that are string
constants, consuming explicit prepared string/data authority without treating
string literals as frame slots or ordinary globals.

## Goal

Move at least one representative string-constant local-memory row past the
current `unsupported_local_memory_access` owner, or reclassify the family to a
more precise producer/policy owner with current diagnostics.

## Core Rule

String-constant local-memory accesses must be driven by explicit prepared
string/data facts. RV64 object emission must fail closed when string identity,
label, extent, byte contents, offset, width, address space, or selected
local-memory use authority is missing, ambiguous, or only inferable from source
spelling, final assembly shape, or testcase identity.

## Read First

- `ideas/open/630_string_constant_local_memory_policy.md`
- `ideas/closed/614_rv64_pointer_local_memory_consumption.md`
- `ideas/closed/308_rv64_string_literals_and_extern_calls.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
- Prepared data and value-location definitions that describe string constants,
  data identity, labels, extents, byte payloads, and local-memory use
  authority.
- RV64 local-memory consumers in
  `src/backend/mir/riscv/codegen/object_emission.cpp`, especially selected
  local-memory access rejection and address/source classification paths.

## Current Targets

- Representative string-constant rows named by the source idea:
  - `src/20000722-1.c`
  - `src/20010123-1.c`
  - `src/20011109-2.c`
  - `src/20021204-1.c`
  - `src/20030920-1.c`
  - `src/920429-1.c`
  - `src/930429-1.c`
  - `src/pr34415.c`
  - `src/pr35800.c`
  - `src/ptr-arith-1.c`
- Prepared facts for string constant identity, label, data extent, byte
  contents when required by the carrier, selected offset, access width, address
  space, and local-memory use authority.
- RV64 object-emission guards that currently reject these rows as
  `unsupported_local_memory_access`.

## Non-Goals

- Generic frame-slot local-memory support already closed by idea 614.
- Direct global-symbol local-memory policy covered by idea 631.
- Aggregate stack-home local-memory policy covered by idea 633.
- Large selected pointer offset local-memory policy covered by idea 634.
- BIR string/data producer repair unless refreshed diagnostics prove this idea
  cannot proceed without a separate producer split.
- Pointer arithmetic policy beyond the explicit string-constant selected-base
  checks needed here.
- ABI, runtime/library policy, expectation changes, unsupported marker changes,
  allowlists, timeouts, or accounting.

## Working Model

- Evidence comes first. Refresh the representative rows before selecting a
  producer or consumer edit.
- String constants are their own prepared-data family. They must not be folded
  into frame-slot or direct-global handling to reuse an unrelated admission
  path.
- Existing prepared carriers should be reused when they already expose the
  needed string identity, storage label, extent, offset, width, and authority.
- Consumer admission should be narrow and fact-driven. It may unblock a row
  only when explicit prepared string/data and selected local-memory authority
  exists.
- Reclassify rows that prove to be producer gaps, global-symbol policy,
  aggregate-home policy, large-offset policy, runtime/library gaps, or pointer
  arithmetic gaps instead of broadening this idea.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Do not edit the source idea unless durable intent changes or closure notes
  are required.
- Add focused prepared-layer or RV64 object-emission tests for any code-changing
  step.
- Use `cmake --build --preset default` plus a supervisor-selected backend
  subset as the normal proof ladder for code slices.
- Treat expectation rewrites, unsupported-marker changes, testcase-shaped
  source matching, filename matching, final-assembly inference, and literal
  spelling inference as route failures.

## Step 1: Refresh String-Constant Evidence

Goal: identify the current string-constant local-memory blockers for the
representative rows.

Actions:
- Re-run focused probes for the rows named in the source idea and any
  immediately adjacent string-constant local-memory rows the diagnostics
  reveal.
- Capture prepared dumps, RV64 object-route diagnostics, selected base kind,
  access width, offset, address space, and current failure bucket labels.
- Distinguish string-constant local-memory gaps from frame-slot, direct-global,
  aggregate-home, large-offset, pointer arithmetic, runtime/library, and
  producer gaps.
- Record evidence and the next owner bucket in `todo.md`.

Completion check:
- `todo.md` lists the rows inspected, the current prepared string/data facts,
  the RV64 rejection or emission point, and a recommended next step that is not
  named-case-only.

## Step 2: Trace String/Data Authority Carriers

Goal: locate the producer and carrier boundary that should provide explicit
string-constant local-memory authority.

Actions:
- Trace prepared string/data production and selected local-memory access
  construction for the Step 1 in-scope bucket.
- Identify the carrier fields that already hold, or should hold, string
  identity, label, extent, bytes, offset, width, address space, and
  local-memory use authority.
- Locate the first missing authority boundary before RV64 object emission.
- Record fail-closed conditions for missing, ambiguous, mismatched, or
  unsupported string-constant local-memory facts.

Completion check:
- `todo.md` names the exact producer functions, carrier fields, consumer
  checks, and the smallest code-changing packet that can publish or consume
  explicit string-constant local-memory authority.

## Step 3: Publish Or Verify Prepared String-Constant Facts

Goal: ensure prepared-layer facts are explicit for one in-scope
string-constant local-memory family before RV64 lowering depends on them.

Actions:
- If upstream string/data authority exists but is not published, add the narrow
  producer publication path.
- If publication already exists, add focused coverage proving the facts are
  complete before object emission.
- Preserve string identity, label, extent, byte payload when needed, selected
  offset, width, address space, and local-memory use authority.
- Do not infer facts from source filenames, literal spelling, final assembly
  shape, or existing row expectations.

Completion check:
- Focused backend coverage proves the selected string-constant local-memory
  facts are present in the prepared representation, or `todo.md` reclassifies
  the bucket with precise missing upstream authority.

## Step 4: Add Narrow RV64 String-Constant Consumer Admission

Goal: allow RV64 object emission to consume only explicit prepared
string-constant local-memory facts for the selected family.

Actions:
- Replace the broad `unsupported_local_memory_access` rejection for the
  selected family with validation of prepared string/data and local-memory
  authority facts.
- Emit only when selected base kind, string identity, label, extent, offset,
  access width, address space, and local-memory use authority match the
  consumer contract.
- Add fail-closed tests for absent, malformed, mismatched, ambiguous, or
  unsupported string/data facts and for frame-slot, direct-global,
  aggregate-home, large-offset, and unrelated owners.
- Preserve existing frame-slot local-memory behavior and keep direct-global,
  aggregate-home, and large-offset families out of this admission path.

Completion check:
- Focused positive and negative backend tests pass, and the RV64 path rejects
  string-constant local-memory accesses without explicit prepared string/data
  authority.

## Step 5: Reclassify Representative Rows

Goal: determine whether the source idea is complete, needs another
string-constant policy packet, or should split remaining work into separate
initiatives.

Actions:
- Re-run the representative row probes after any Step 3 or Step 4 changes.
- Classify each remaining failure into string-constant local-memory authority
  or an out-of-scope owner bucket.
- Record whether idea 630 is close-ready or which next string-constant
  local-memory packet is justified.

Completion check:
- `todo.md` contains row-by-row classification, proof results, and a clear
  close/split/continue recommendation for the supervisor.
