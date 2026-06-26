# RV64 Global Aggregate Lane Materialization Runbook

Status: Active
Source Idea: ideas/open/383_rv64_global_aggregate_lane_materialization.md

## Purpose

Activate the direct follow-up from closed idea 382 for the next
`src/20030914-2.c` RV64 object-route boundary:

```text
%t0.global.aggregate.load.0 = bir.load_local i32 %t0.0, addr gs
```

## Goal

Route or lower global aggregate lane loads from explicit prepared data facts,
or prove the missing fact belongs to an upstream prepared-data publication
owner.

## Core Rule

Consume prepared global/data facts and object-model symbol, section, relocation,
and address-use metadata. Do not reconstruct aggregate bytes, global identity,
or source layout from testcase names, C syntax, raw offsets, physical registers,
or log text.

## Read First

- `ideas/open/383_rv64_global_aggregate_lane_materialization.md`
- `ideas/closed/357_rv64_object_route_data_sections_globals_strings.md`
- `ideas/closed/382_rv64_object_route_prepared_local_memory_addressing.md`
- The prepared dump for `src/20030914-2.c`, especially `main` insts
  `0,2,...,34` and the paired destination frame-slot stores.
- RV64 object-emission tests for globals, strings, relocations, local memory,
  and rejected unsupported object-route shapes.

## Current Targets

- Representative gcc torture case: `src/20030914-2.c`
- Focused backend fixtures for global aggregate data and data-symbol memory
  access
- RV64 object-emission path currently diagnosing the global-source lane loads
  through `unsupported_local_memory_access`
- Prepared BIR/module data publication for aggregate globals

## Non-Goals

- Do not reopen byval aggregate parameter homes or prepared local-memory
  pointer-value/frame-slot lanes; ideas 370 and 382 closed those boundaries.
- Do not reopen blanket prepared globals, strings, ELF data sections, symbols,
  or relocations already closed by idea 357.
- Do not implement aggregate `va_arg` helper lowering; that belongs to
  `ideas/open/371_rv64_object_route_aggregate_va_arg_helper_lowering.md`.
- Do not treat global-source lanes as stack local memory to satisfy the old
  diagnostic.
- Do not add source-name handling for `src/20030914-2.c` or global-name
  handling for `gs`.
- Do not weaken expectations, allowlists, unsupported contracts, or diagnostics
  to claim progress.

## Working Model

The representative now reaches a global aggregate source copied lane-by-lane
into a frame-slot destination. The destination stores are prepared
frame-slot-based, but the source `load_local ... addr gs` lanes do not publish
the frame-slot or pointer-value metadata required by the local-memory object
route. The first packet should decide whether the source global data facts are
already available elsewhere in prepared module metadata or missing upstream. If
they are available, object emission should lower them through semantic
global/data object-model machinery. If they are missing, the route should fail
closed with a precise upstream-owner diagnostic.

## Execution Rules

- Keep each implementation packet narrow enough to prove with a build and a
  focused RV64 object-route test subset.
- Prefer focused backend tests before rerunning broad gcc torture scans.
- Preserve or improve unsupported diagnostics when the missing fact is upstream.
- Treat testcase-specific matching for `src/20030914-2.c` or `gs` as route
  drift.
- Escalate to supervisor review if the needed prepared global/data facts are
  absent; do not infer them in target emission.
- Use the canonical `test_after.log` only when delegated by the supervisor for
  executor proof.

## Steps

### Step 1: Audit Global Aggregate Lane Facts

Goal: identify the exact prepared facts available for the `main` `addr gs`
aggregate lane loads.

Primary target: prepared BIR/module dumps for `src/20030914-2.c` and the RV64
object-route rejection path.

Actions:

- Locate the diagnostic site reached by `bir.load_local i32 ... addr gs`.
- Trace the source global facts available there: symbol identity, section,
  initializer bytes or lane payload, offset, access size, alignment, address
  materialization, relocation needs, and address space.
- Compare those facts with the object/data support closed by idea 357.
- Record in `todo.md` whether this is target-consumable prepared data or an
  upstream prepared-data publication gap.

Completion check:

- The executor can name the first supportable or rejected global aggregate
  lane shape using only prepared facts, without source syntax or testcase
  identity.

### Step 2: Add Focused Supported Or Rejected Fixtures

Goal: encode the selected global aggregate lane boundary in focused tests.

Primary target: RV64 object-emission tests and any prepared-printer or
diagnostic coverage needed to prove the data facts.

Actions:

- Add or extend focused tests for global aggregate lane loads from explicit
  prepared facts if the facts are target-consumable.
- Add rejected-shape coverage for missing global data facts, unsupported
  widths, non-default address spaces, dynamic offsets, ambiguous aggregate
  storage, or out-of-bounds lane offsets as applicable.
- Keep test names and expectations semantic; do not key behavior to
  `src/20030914-2.c` or `gs`.

Completion check:

- Focused tests either fail for missing semantic support or prove a narrower
  fail-closed upstream-publication diagnostic before any lowering change is
  accepted.

### Step 3: Implement Or Route Global Aggregate Lane Materialization

Goal: lower the selected global aggregate lane shape, or route the missing fact
to the correct upstream owner.

Primary target: RV64 object-route data-symbol access, relocation, and memory
fragment helpers, or the prepared-data producer if target facts are missing.

Actions:

- If complete prepared facts are present, add or reuse helpers that materialize
  the global data address and emit lane loads through valid RV64 object-model
  symbol/section/relocation semantics.
- If facts are missing, preserve a precise unsupported diagnostic naming the
  missing prepared global aggregate data fact.
- Preserve fail-closed behavior for unsupported widths, non-default address
  spaces, dynamic offsets, ambiguous aggregate storage, and out-of-bounds lane
  offsets.
- Keep helper boundaries named around semantic prepared data facts, not the
  representative testcase.

Completion check:

- Focused supported tests pass because of semantic global/data materialization,
  or focused rejected tests prove the precise upstream-publication boundary.

### Step 4: Rerun Representative And Route Next Boundary

Goal: prove whether `src/20030914-2.c` advances beyond the `main` `addr gs`
lanes and document the next owner.

Primary target: the narrow RV64 gcc torture object runner for
`src/20030914-2.c`.

Actions:

- Rerun the representative using the repo-native narrow RV64 gcc torture
  object-route command selected by the supervisor.
- Document whether the case passes or advances to aggregate `va_arg`,
  terminator, call, control-flow, or another distinct owner.
- If a new distinct initiative is exposed, report it to the supervisor instead
  of silently expanding this plan.

Completion check:

- `todo.md` records the representative outcome, the proof command, and whether
  the next boundary belongs to this idea or a separate open idea.

### Step 5: Broader Guard And Closure Review

Goal: prepare the slice for supervisor acceptance and eventual source-idea
closure.

Primary target: existing RV64 object-emission, object/data, relocation, and
prepared-printer coverage.

Actions:

- Run the focused proof required for the implementation packet.
- Run broader RV64 object-emission, object/data, or prepared-contract coverage
  if the helper touches shared data-symbol or address materialization.
- Confirm no expectations, allowlists, or unsupported contracts were weakened.
- Summarize remaining unsupported global/data shapes and why they are outside
  this runbook or need a separate idea.

Completion check:

- The implementation has fresh build/test proof, no testcase-overfit evidence,
  and clear notes for any remaining global aggregate/data materialization gaps.
