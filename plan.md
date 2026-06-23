# RV64 Local Stack Slot And Address Materialization Plan

Status: Active
Source Idea: ideas/open/312_rv64_local_stack_slot_address_materialization.md

## Purpose

Repair RV64 lowering for local stack slots, address-taken locals, arrays,
aggregates, pointer values, and function-pointer storage so local effective
addresses and loaded values are materialized before use.

## Goal

Make representative local stack/address c-testsuite cases emit, link, and run
under qemu by repairing general RV64 stack slot and effective-address lowering.

## Core Rule

Do not special-case candidate filenames, variable names, stack offsets, or
known emitted shapes. Progress must come from semantic local address/materialized
value lowering and focused backend coverage.

## Read First

- ideas/open/312_rv64_local_stack_slot_address_materialization.md
- build/rv64_c_testsuite_probe_latest/triage_step4/summary.md, if present
- build/rv64_c_testsuite_probe_latest/triage_311_step6_acceptance/probe_results.tsv, if present
- RV64 prepared local memory/scalar/function emission files under
  `src/backend/mir/riscv/codegen/`
- Existing backend RV64 prepared tests under `tests/backend/case/`

## Scope

- Local stack object allocation and stack slot address publication.
- Loads and stores through local effective addresses.
- Array element addressing and aggregate-local access.
- Pointer-to-local and address-taken local flow.
- Function-pointer local storage and use.
- Recent residual `src/00143.c` as a classification/probe target when its local
  array or address-offset behavior overlaps this source idea.

## Non-Goals

- Global object storage or global address repair beyond isolation checks.
- External libc/libm/string/user external call policy.
- Generic scalar width cleanup unless required for local address proof.
- Expectation rewrites, unsupported downgrades, or qemu-skip changes.
- Fake labels, epilogues, or fall-throughs that hide missing local
  materialization.

## Working Model

The triage bucket points to stack/local lowering that loses either an effective
address or the loaded value before use. Representative bad evidence includes
suspicious stack loads in `src/00005.c`, incomplete array pointer/indexed use in
`src/00032.c`, and incomplete array element address/use checks in `src/00130.c`.
Treat runtime crashes as symptoms; inspect prepared BIR, prepared facts, and
emitted assembly to find the first missing semantic lowering rule.

## Execution Rules

- Start every implementation step with a focused before/after probe of the
  delegated candidate cases and nearby same-feature cases.
- Add backend tests that express the semantic feature, not the c-testsuite
  filename.
- Prefer repairing existing local memory/address APIs or RV64 emission rules
  over adding local hacks in the final assembly path.
- Keep candidate reclassification concrete: cite emitted assembly, BIR/prepared
  facts, and qemu/clang behavior.
- Use `test_after.log` for executor proof unless the supervisor delegates a
  different artifact, and leave canonical regression-log policy to the
  supervisor.

## Step 1: Normalize Local Address Evidence

Goal: split the local stack/address bucket into concrete repair families.

Primary targets:

- `src/00005.c`
- `src/00032.c`
- `src/00072.c`
- `src/00130.c`
- `src/00143.c` as a residual overlap probe

Actions:

- Reprobe the primary targets for emit, clang, and qemu outcome.
- Capture prepared BIR/control/data facts and emitted RV64 assembly for each
  representative.
- Identify the first bad materialization point for pointer-to-local, array
  indexing, aggregate-local, and function-pointer local flows.
- Record which candidates are true local stack/address failures and which
  belong to another source idea.

Completion check:

- `todo.md` names the chosen first repair family, representative tests, and any
  reclassified cases with concrete evidence.

## Step 2: Cover Local Pointer And Address-Taken Slots

Goal: prove local pointer/address-taken stack slot behavior before changing the
emitter.

Primary target:

- A focused backend case derived from `src/00005.c`-style pointer-to-local flow.

Actions:

- Add or update backend tests for address-of-local, storing/loading the pointer,
  and dereferencing through the local effective address.
- Include a codegen-route check that catches missing or stale stack slot address
  materialization.
- Keep test assertions semantic and resilient to unrelated register allocation
  details.

Completion check:

- Focused tests fail for the current local pointer/address-taken gap and do not
  rely on c-testsuite names.

## Step 3: Repair Local Effective Address Materialization

Goal: lower local effective addresses and loads/stores through them correctly.

Primary target:

- RV64 prepared local memory/address emission.

Actions:

- Inspect the local stack slot metadata, prepared memory facts, and emission
  path that should produce effective addresses from `sp` plus the owned offset.
- Repair address publication and use so loads/stores consume the intended local
  address/value with correct RV64 width.
- Preserve existing successful scalar local behavior.
- Run focused backend tests and a small runtime probe including `src/00005.c`
  and nearby local-pointer candidates.

Completion check:

- The selected local pointer/address-taken representatives emit, link, and pass
  qemu, and the backend subset has no new failures.

## Step 4: Cover And Repair Array Element Local Access

Goal: make local array element addressing and indexed local loads/stores use
materialized local addresses.

Primary targets:

- `src/00032.c`
- `src/00130.c`
- Nearby array candidates from the source idea.

Actions:

- Add focused backend coverage for local array indexing and element
  load/store/update flows.
- Repair indexed effective address formation for stack-backed local arrays.
- Prove both constant-index and value-index shapes when present in the
  candidate evidence.

Completion check:

- Representative local array candidates emit, link, and pass qemu, or any
  residual failure is reclassified with concrete non-local evidence.

## Step 5: Cover Aggregate And Function-Pointer Local Flow

Goal: extend the repaired local materialization path to aggregate-local and
function-pointer local storage/use without broad rewrites.

Primary targets:

- Candidate cases involving aggregate locals and function-pointer local
  storage/use from idea 312.

Actions:

- Add focused backend tests for aggregate-local subobject access and
  function-pointer local store/load/call when the candidate evidence supports
  it.
- Repair only the local materialization rules required by those tests.
- Reprobe affected candidates and classify any remaining failures.

Completion check:

- Focused aggregate/function-pointer local tests pass, and representative
  runtime candidates no longer fail from missing local stack/address
  materialization.

## Step 6: Acceptance Sweep And Reclassification

Goal: decide whether idea 312 is complete or needs a narrowed follow-up.

Primary targets:

- All candidate cases listed in the source idea.
- `src/00143.c` if Step 1 classified it as in-scope local array/address work.

Actions:

- Reprobe the full candidate set for emit, clang, and qemu outcome.
- Summarize pass/fail movement by concrete mechanism.
- Reclassify residual failures into follow-up ideas only when they are outside
  local stack/address materialization.
- Run the supervisor-selected backend guard or broader validation checkpoint.

Completion check:

- Acceptance criteria in the source idea are satisfied, residuals are
  concretely classified, and the lifecycle owner can decide whether to close
  the idea.
