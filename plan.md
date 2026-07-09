# Select Publication Source Wiring

Status: Active
Source Idea: ideas/open/616_select_publication_source_wiring.md

## Purpose

Wire select publication source evidence into the prepared/RV64 boundary for
supported stack-offset and move-bundle rows while preserving the source
freshness, alias evidence, and destination legality distinctions from ideas
`589` and `598`.

Goal: convert current select publication rows with established source
freshness into semantic progress, or classify missing authority into durable
follow-up work.

Core Rule: do not treat alias evidence or destination legality as source
freshness.

## Read First

- `ideas/open/616_select_publication_source_wiring.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
- Closed reference ideas named by the source idea:
  - `ideas/closed/589_direct_edge_publication_move_freshness_ownership.md`
  - `ideas/closed/598_select_carrier_alias_freshness_contract.md`

## Current Scope

- Select publication source wiring where freshness is already established.
- Stack-offset source rows with complete source-freshness authority.
- Move-bundle select publication wiring rows with complete source-freshness
  authority.
- Candidate split-in rows such as `src/921124-1.c` and `src/920710-1.c` only
  after refreshed diagnostics prove they are select publication rows with the
  required freshness and destination legality.

## Non-Goals

- General select instruction lowering.
- Destination fan-in authority.
- Branch stack-source, pointer local-memory, ABI, runtime, expectation,
  unsupported-marker, allowlist, timeout, or accounting changes.
- Treating alias evidence, destination legality, or testcase names as a
  substitute for source freshness.

## Working Model

Ideas `589` and `598` define the boundary: source freshness, alias evidence,
and destination legality are separate facts. This plan may consume established
freshness for select publication, but it must fail closed when only alias or
destination evidence is present.

## Execution Rules

- Keep routine packet findings in `todo.md`.
- Step 1 must refresh diagnostics before implementation.
- If diagnostics show missing source freshness or missing destination
  authority, split or classify that work instead of inferring facts in RV64.
- Add or update focused tests for semantic authority paths, not named-case
  shortcuts.
- Use backend proof for any code-changing packet:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
- Keep focused object-emission or prepared-dump probes as supporting evidence,
  not as substitutes for the delegated proof command when implementation
  changes.

## Step 1: Refresh Select Publication Residuals

Goal: identify current select publication residual rows and their first owners
before implementation.

Actions:
- Refresh the select publication stack-offset and move-bundle rows named by
  the current failure map.
- Record row counts, representative testcase names, first owner, and whether
  each row has explicit source freshness.
- Separate rows that only have alias evidence or destination legality from
  rows with true source-freshness authority.
- Specifically classify `src/921124-1.c` and `src/920710-1.c` as select
  publication, terminator, branch stack-source, or another owner before any
  code changes.

Completion Check:
- `todo.md` records the refreshed owner families and identifies either one
  in-scope select publication consumer packet or a concrete split/classify
  decision.

## Step 2: Wire One Proven Select Publication Source Path

Goal: implement one bounded source-freshness consumer path proven by Step 1.

Actions:
- Touch only the prepared/RV64 boundary surface needed for the audited source
  path.
- Preserve fail-closed behavior for missing source freshness, alias-only
  evidence, and destination-only legality.
- Add focused positive and negative tests for the semantic authority contract.
- Do not mix branch stack-source, destination fan-in, generic select lowering,
  terminator, or runtime changes into this packet.

Completion Check:
- Multiple rows with complete source-freshness authority move past the selected
  publication owner or are reclassified with concrete evidence.
- Guard rows lacking source freshness remain rejected.
- Backend proof passes with the delegated `^backend_` command.

## Step 3: Refresh Residuals And Split Decision

Goal: determine whether more in-scope select publication work remains.

Actions:
- Refresh select publication residuals after Step 2 or after a Step 1
  no-implementation decision.
- Classify remaining rows as same-owner, separate owner, or downstream
  non-select-publication work.
- If remaining work is separate, create or request durable follow-up ideas
  rather than expanding this runbook.

Completion Check:
- `todo.md` records remaining row families, exclusions, and the recommended
  next lifecycle action.

## Step 4: Close-Readiness Classification

Goal: decide whether idea `616` is complete, blocked, or needs a runbook
rewrite.

Actions:
- Verify the source idea's acceptance criteria against current diagnostics.
- Confirm no complete-authority select publication consumer family remains
  unhandled in this idea.
- Prepare lifecycle notes for closure, split, or rewrite if residual work
  belongs elsewhere.

Completion Check:
- Plan owner can decide close, split, or rewrite from `todo.md` without
  re-running the whole audit.
