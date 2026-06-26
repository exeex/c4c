# Prepared Global-Symbol Memory Access Publication Runbook

Status: Active
Source Idea: ideas/open/384_prepared_global_symbol_memory_access_publication.md

## Purpose

Activate the upstream follow-up from closed idea 383 for the current
`src/20030914-2.c` RV64 object-route boundary:

```text
unsupported_global_data: RV64 object route requires prepared global-symbol memory access facts for LoadLocalInst global-address lanes
```

## Goal

Publish prepared global-symbol memory-access facts for BIR global-address load
lanes so targets can consume semantic data facts instead of inferring from raw
`LoadLocalInst addr <global>` spelling.

## Core Rule

Producer-side prepared facts own symbol identity, lane offset, access size,
alignment, address space, section/storage, and relocation/address-use meaning.
Target emission may consume those facts, but must not reconstruct them from
testcase names, source syntax, raw log text, or diagnostic strings.

## Read First

- `ideas/open/384_prepared_global_symbol_memory_access_publication.md`
- `ideas/closed/383_rv64_global_aggregate_lane_materialization.md`
- `ideas/closed/357_rv64_object_route_data_sections_globals_strings.md`
- The prepared dump for `src/20030914-2.c`, especially `main`, block `entry`,
  inst `0` and repeated global aggregate source lanes through offset `68`
- Existing prepared-printer, prepared-contract, and RV64 object-emission tests
  around globals, data sections, memory-access facts, and fail-closed raw
  global-address lanes

## Current Targets

- Representative gcc torture case: `src/20030914-2.c`
- Prepared-data producer path for `LoadLocalInst` global-address lanes
- Prepared memory-access records consumed by RV64 object emission
- Focused prepared-contract and object-route fixtures for global-symbol data
  lanes

## Non-Goals

- Do not add target-side fallback inference from raw
  `LoadLocalInst addr <global>` spelling.
- Do not reopen broad ELF data sections, globals, strings, symbols, or
  relocations already closed by idea 357.
- Do not implement aggregate `va_arg` helper lowering; that belongs to
  `ideas/open/371_rv64_object_route_aggregate_va_arg_helper_lowering.md`.
- Do not work on non-register parameter homes; that belongs to
  `ideas/open/374_rv64_object_route_non_register_param_homes.md`.
- Do not key behavior to `src/20030914-2.c`, global `gs`, or lane offset `68`
  except as representative proof data.

## Working Model

The RV64 object route now has a precise target-side failure for raw global lanes
and focused support for explicit prepared `GlobalSymbol` access facts. The
remaining work is to make the prepared-data producer publish those facts for
supportable global-address loads before target emission runs.

## Execution Rules

- Keep implementation packets small and prove producer changes with focused
  prepared-contract or prepared-printer tests before rerunning the representative.
- Preserve the idea 383 fail-closed target diagnostic for missing prepared facts.
- Publish semantic facts from the producer/contract layer; do not patch RV64
  object emission to guess them.
- Record any newly exposed representative boundary in `todo.md` and route it to
  an existing or new idea instead of expanding this runbook silently.
- Use canonical `test_after.log` only when delegated by the supervisor for
  executor proof.

## Steps

### Step 1: Audit Producer Source For Global Lanes

Goal: identify the producer path that emits `LoadLocalInst` global-address
lanes without prepared global-symbol memory-access facts.

Primary target: prepared-data publication for `src/20030914-2.c` `main`,
`entry`, inst `0`.

Actions:

- Trace where the BIR `LoadLocalInst` global-address lane is produced.
- List which facts are already available there: global symbol, lane offset,
  access width, alignment, address space, initializer/storage relation, and
  relocation/address-use meaning.
- Compare available facts with the explicit `GlobalSymbol` access facts already
  consumed by RV64 object emission.
- Record whether the first supportable shape can be published directly or needs
  a narrower upstream contract prerequisite.

Completion check:

- `todo.md` names the exact producer surface and the first complete or missing
  global-symbol memory-access fact set.

### Step 2: Add Prepared-Contract Coverage

Goal: encode the selected publication boundary in focused tests.

Primary target: prepared-printer, prepared-contract, or producer tests that can
prove global-symbol memory-access publication before target lowering.

Actions:

- Add coverage for the first supportable global-symbol lane load shape.
- Add fail-closed coverage for missing or ambiguous publication facts such as
  unknown symbol identity, dynamic offset, unsupported width, non-default
  address space, or incomplete relocation/address-use meaning.
- Keep tests semantic and independent of `src/20030914-2.c` or `gs`.

Completion check:

- Focused tests prove the expected publication facts or the precise unsupported
  producer boundary before implementation is accepted.

### Step 3: Publish Global-Symbol Memory-Access Facts

Goal: publish prepared global-symbol memory-access facts for supportable global
data load lanes.

Primary target: the producer/contract layer identified in Step 1.

Actions:

- Add or extend the prepared data structure that records global-symbol memory
  accesses for load lanes.
- Populate symbol identity, offset, size, alignment, address space, and
  relocation/address-use semantics from authoritative producer facts.
- Ensure target RV64 object emission continues to consume explicit prepared
  facts and keeps raw global-address lanes fail-closed when publication is
  absent.
- Preserve precise unsupported diagnostics for incomplete or ambiguous
  publication shapes.

Completion check:

- Focused producer and RV64 object-emission tests pass because the semantic
  prepared fact exists, not because target emission inferred from raw spelling.

### Step 4: Rerun Representative And Route Next Boundary

Goal: prove whether `src/20030914-2.c` advances beyond the global-source lane
publication boundary.

Primary target: the narrow RV64 gcc torture object runner for
`src/20030914-2.c`.

Actions:

- Rerun the representative using the supervisor-selected command.
- Document whether the case passes or advances to aggregate `va_arg`,
  terminator, call, control-flow, parameter-home, or another distinct owner.
- If a new distinct initiative is exposed, report it instead of broadening this
  plan.

Completion check:

- `todo.md` records the representative outcome, proof command, and next owner.

### Step 5: Broader Guard And Closure Review

Goal: prepare this upstream publication slice for supervisor acceptance and
eventual source-idea closure.

Primary target: prepared-contract, object-emission, and representative coverage.

Actions:

- Run the focused proof required for the implementation packet.
- Run broader prepared-contract or backend coverage if the publication structure
  is shared by other data or memory-access shapes.
- Confirm no expectations, allowlists, unsupported contracts, or target-side
  inference paths were weakened.
- Summarize remaining prepared global-symbol publication gaps and route them to
  existing or new ideas.

Completion check:

- The implementation has fresh build/test proof, no testcase-overfit evidence,
  and clear lifecycle notes for any remaining owners.
