# Aggregate Stack-Home Local-Memory Policy

Status: Open
Type: Implementation
Parent: `ideas/closed/614_rv64_pointer_local_memory_consumption.md`
Related:
- `ideas/closed/614_rv64_pointer_local_memory_consumption.md`
- `ideas/open/624_prepared_outgoing_stack_argument_destination_offsets.md`
- `ideas/open/627_pointer_stack_result_call_policy.md`
- `ideas/open/629_prepared_return_destination_home_authority.md`
Owning Layer: aggregate/sret/byval stack-home local-memory policy
Queue Order: 33
Prerequisites: aggregate, sret, byval, or pointer stack-home facts must
identify source value, destination home, stack slot, offset, size, and memory
use authority before RV64 local-memory emission
Proof Surface: local-memory rows involving sret/byval or aggregate pointer
stack homes that remain outside idea 614's frame-slot consumer support

## Goal

Define the policy and authority boundary for local-memory accesses through
aggregate, sret, byval, or pointer stack homes without inferring stack-home
semantics inside the generic RV64 local-memory consumer.

## Why This Exists

Idea 614 closed after supporting explicit prepared frame-slot local-memory
facts. Step 4 residuals kept sret/byval and aggregate pointer stack-home rows
out of 614 because they require stack-home policy and producer authority beyond
plain selected frame-slot access.

## In Scope

- Refresh residual local-memory rows that access aggregate, sret, byval, or
  pointer stack homes.
- Separate producer-publication gaps from RV64 consumer gaps using explicit
  stack-home, offset, size, value identity, and memory-use facts.
- Implement RV64 consumption only when those facts are complete and shared
  across a row family.
- Keep diagnostics fail-closed for missing stack-home authority, ambiguous
  aggregate lanes, incomplete size/offset facts, and stale value identity.

## Out Of Scope

- Outgoing stack argument destination publication owned by idea `624`.
- Pointer stack-result call policy owned by idea `627`.
- Prepared return destination-home authority owned by idea `629`.
- Generic frame-slot local-memory support already closed by idea 614.
- String/global local-memory policies, F128 width policy, expectations,
  unsupported markers, allowlists, timeouts, runtime, or accounting.

## Acceptance Criteria

- A refreshed probe identifies whether aggregate stack-home residuals belong
  to producer authority, ABI policy, return policy, or RV64 local-memory
  consumption.
- At least one complete-authority aggregate stack-home local-memory family
  moves past its current owner, or all such rows are reclassified into more
  precise existing/new producer ideas.
- Negative proof keeps plain frame-slot, string/global, 16-byte/F128, ABI
  transport, and return-only rows outside this policy.

## Reviewer Reject Signals

- Reject RV64 inference of aggregate stack homes from final assembly layout,
  ABI convention, source filenames, or object shape.
- Reject testcase-shaped handling for a single aggregate/sret/byval row.
- Reject broad ABI or return rewrites that bypass ideas `624`, `627`, or
  `629`.
- Reject expectation, unsupported-marker, allowlist, timeout, runtime, or
  accounting changes as capability progress.
- Reject helper renames or diagnostic wording changes that leave stack-home
  local-memory authority missing.
