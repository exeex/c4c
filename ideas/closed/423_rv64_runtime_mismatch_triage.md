# RV64 Runtime Mismatch Triage

Status: Closed
Type: Runtime correctness triage idea
Parent: `ideas/open/420_rv64_gcc_torture_post_contract_umbrella.md`
Owning Layer: RV64/MIR runtime behavior
Handoff Directory: `docs/rv64_gcc_torture_post_contract/`

## Goal

Classify RV64 gcc_torture rows that compile, link, and run but diverge from
clang through aborts, segfaults, exit-code mismatches, or output mismatches.

## Why This Exists

Runtime mismatches are more dangerous than fail-closed unsupported routes
because the backend emits executable code that behaves incorrectly. They should
be triaged before niche F128 feature work.

## In Scope

- Reproduce current runtime mismatch rows from RV64 gcc_torture.
- Group failures by abort, segfault, wrong exit code, wrong output, and timeout
  if applicable.
- Use disassembly and focused prepared/object evidence to identify first wrong
  value, address, branch, call, or storage home.
- Produce follow-up implementation ideas for broad runtime bug families.

## Out Of Scope

- Fixing runtime mismatches in this triage idea.
- Weakening runtime comparison.
- Treating unsupported compile failures as runtime mismatches.
- F128-specific work unless it is proven to own a runtime mismatch family.

## Acceptance Criteria

- Runtime mismatch rows have concrete ownership notes and representative
  artifacts.
- At least one high-confidence runtime implementation idea is proposed if the
  bucket has a recurring owner.
- Rows with unclear ownership remain fail-closed for further triage rather than
  receiving speculative patches.
- Default CTest remains stable.

## Completion Notes

Closed after the Step 1-5 triage runbook produced durable handoff artifacts:

- `docs/rv64_gcc_torture_post_contract/runtime_mismatch_inventory.md`
- `docs/rv64_gcc_torture_post_contract/runtime_mismatch_representatives.md`
- `docs/rv64_gcc_torture_post_contract/runtime_first_wrong_edge.md`
- `docs/rv64_gcc_torture_post_contract/runtime_mismatch_groups.md`

The triage found 40 runtime mismatch rows in the freshest full-scan artifacts,
reproduced representative rows, and recorded first-wrong-edge evidence for
three live representatives. Two ordinary-C implementation routes were split
into separate open follow-up ideas:

- `ideas/open/443_rv64_prepared_value_operand_materialization.md`
- `ideas/open/444_frame_slot_call_argument_publication.md`

Unclear runtime rows remain parked rather than speculatively assigned: abort
rows outside `src/pr38533.c`, segfault rows outside `src/20080506-2.c`, and
the stale or nondeterministic `src/20000314-2.c` row. The inline-asm
tied-output/result materialization group from `src/pr38533.c` is documented as
implementation-ready but deferred as lower ordinary-C priority unless a later
supervisor packet activates that route.

Close-time default CTest stability was checked with c4c regression guard using
matching default CTest before/after logs and non-decreasing pass count policy
for this lifecycle/docs-only close.

## Reviewer Reject Signals

- Reject output-only or exit-code expectation changes.
- Reject source-name, testcase-name, function-name, or block-index fixes.
- Reject claiming progress by changing unsupported diagnostics.
- Reject broad runtime rewrites without first-wrong-edge evidence.
- Reject F128-driven routing unless the bucket evidence supports it.
