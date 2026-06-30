# RV64 Runtime Mismatch Triage

Status: Open
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

## Reviewer Reject Signals

- Reject output-only or exit-code expectation changes.
- Reject source-name, testcase-name, function-name, or block-index fixes.
- Reject claiming progress by changing unsupported diagnostics.
- Reject broad runtime rewrites without first-wrong-edge evidence.
- Reject F128-driven routing unless the bucket evidence supports it.
