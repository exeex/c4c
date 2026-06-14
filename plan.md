# Control-Flow Block-Index Label Bridge Runbook

Status: Active
Source Idea: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md

## Purpose

Activate one remaining idea 260 candidate: the `control_flow` block-index
label bridge packet.

## Goal

Allow `prepared_block_label_for_index` to use a BIR block-id agreement fact
only when the prepared control-flow row and BIR block structure agree, while
preserving current prepared fallback and invalid-label behavior.

## Core Rule

This runbook owns exactly one candidate. Do not migrate unrelated
`module`, `names`, `control_flow`, or `store_source_publications` readers, and
do not delete, privatize, wrap, or broadly retire `PreparedBirModule` fields.

## Read First

- `ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md`
- Existing `prepared_block_label_for_index` implementation and its direct
  helper surface.
- Focused backend helper tests that already cover prepared lookup agreement and
  fail-closed rows.

## Current Target

- Candidate: `control_flow` block-index label bridge packet.
- Reader/helper row: `prepared_block_label_for_index`.
- Semantic fact: BIR block-id / prepared block-label agreement for one block
  index.
- Retained compatibility surface: public prepared control-flow labels and
  current invalid-label/fallback behavior.

## Non-Goals

- Do not reactivate completed `module` lookup-reader, `module` top-level
  printer, `names` same-block value-name, `names` value-home,
  `names` semantic resolver, `control_flow` call-preservation, or
  `control_flow` branch-target packets.
- Do not alter printer/debug output, route-debug strings, helper/oracle status
  names, target output, expectation baselines, or unsupported markers.
- Do not change broad CFG traversal, branch lowering, publication planning, or
  store-source behavior.
- Do not weaken public prepared aggregate compatibility.

## Working Model

`prepared_block_label_for_index` may consult BIR structure only behind an
agreement boundary. The helper must fail closed when either side is absent,
short, invalid, stale, or mismatched. Existing prepared fallback and
`kInvalidBlockLabel` behavior remain authoritative when agreement is not
complete.

## Execution Rules

- Keep implementation scope local to the block-index label bridge helper and
  focused tests.
- Prefer a local/private agreement helper unless an existing public helper
  surface already matches the needed contract.
- Add proof rows for the nearby failure family, not just one positive case.
- Treat expectation rewrites or named-case shortcuts as route drift.
- Use `test_after.log` for executor proof output unless the supervisor
  delegates another artifact.

## Steps

### Step 1: Locate the Block-Index Label Bridge Surface

Goal: identify the exact helper, call sites, and existing focused tests for
`prepared_block_label_for_index`.

Primary target: the current `prepared_block_label_for_index` implementation and
its direct focused helper tests.

Actions:

- Inspect the helper and any private control-flow/BIR agreement utilities it
  can reuse.
- Identify how prepared control-flow length, prepared label ids, BIR block
  labels, and invalid labels are represented today.
- Identify the focused helper test file and the smallest proof command for
  this candidate.

Completion check:

- `todo.md` records the selected helper surface, direct tests, and the proof
  command for the next packet.

### Step 2: Implement or Confirm the Agreement Boundary

Goal: make `prepared_block_label_for_index` eligible to use BIR block-id facts
only under complete prepared/BIR agreement.

Primary target: the helper surface found in Step 1.

Actions:

- Add or reuse an agreement check that validates the prepared control-flow row,
  prepared block-label id, BIR block index, and BIR structured block-label id.
- Preserve fallback to existing prepared labels when the BIR fact is absent,
  incomplete, or non-authoritative.
- Preserve current `kInvalidBlockLabel` behavior for invalid or unavailable
  rows.
- Keep the helper behavior-preserving for callers that only observe prepared
  aggregate compatibility.

Completion check:

- The focused helper test builds and existing rows still pass.
- No unrelated implementation or output files are changed.

### Step 3: Add Fail-Closed Proof Rows

Goal: prove the bridge accepts only complete agreement and otherwise keeps
  current fallback or invalid-label behavior.

Primary target: focused backend helper tests for prepared lookups/control-flow.

Actions:

- Add a positive agreement row for the BIR block-id bridge if one is missing.
- Cover control-flow absent.
- Cover control-flow shorter than the queried index.
- Cover BIR shorter than the queried index.
- Cover invalid BIR labels.
- Cover prepared/BIR mismatch.
- Keep prepared-only fallback and invalid-label compatibility observable.

Completion check:

- Focused helper tests pass with the supervisor-selected narrow command.
- Test rows demonstrate the nearby failure family, not a named-case shortcut.

### Step 4: Broader Backend Validation

Goal: prove this one-candidate slice did not regress default backend behavior.

Actions:

- Run the supervisor-selected broader backend subset, expected to be
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
- Preserve the result in `test_after.log`.
- Update `todo.md` with the command, result, and close-readiness notes.

Completion check:

- Broader backend validation passes.
- `todo.md` records whether the active runbook is ready for plan-owner
  retirement review while keeping the source idea open for remaining
  candidates.
