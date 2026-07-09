# F128 And 16-Byte Local-Memory Width Policy

Status: Open
Type: Implementation
Parent: `ideas/closed/614_rv64_pointer_local_memory_consumption.md`
Related:
- `ideas/closed/614_rv64_pointer_local_memory_consumption.md`
- `ideas/open/628_fpr_abi_frame_policy_and_placement.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
Owning Layer: RV64 local-memory width policy
Queue Order: 32
Prerequisites: local-memory access facts must publish width, type class,
alignment, selected base, and whether the access is F128/long-double or a
non-F128 16-byte aggregate access
Proof Surface: unsupported 16-byte/F128 local-memory rows such as
`src/20010605-2.c`, `src/20040208-1.c`, and `src/ieee/inf-1.c`

## Goal

Classify and define policy for 16-byte local-memory accesses, separating
primary F128/long-double work from any ordinary aggregate-width local-memory
consumer support.

## Why This Exists

Idea 614 intentionally supported only explicit prepared frame-slot
local-memory accesses with 1/2/4/8-byte scalar or floating widths. Step 4
residuals identified unsupported 16-byte/F128 width rows that should not be
absorbed into the narrow frame-slot consumer route.

`src/complex-7.c` has mixed global-symbol and 16-byte evidence and should be
kept as a guard until diagnostics cleanly assign it to one owner.

## In Scope

- Refresh unsupported 16-byte local-memory rows and classify F128/long-double,
  complex/vector, and ordinary aggregate-width shapes.
- Define fail-closed RV64 policy for unsupported widths when explicit support
  is not implemented.
- Implement a semantic 16-byte local-memory consumer only if refreshed
  diagnostics prove broad non-F128 authority and alignment support.
- Keep F128-specific work isolated from broad ordinary-C recovery unless the
  refreshed evidence proves it blocks non-F128 coverage.

## Out Of Scope

- FPR ABI/frame placement covered by idea `628`.
- String-constant and direct global-symbol local-memory policies covered by
  ideas `630` and `631`.
- Aggregate stack-home local-memory policy covered by idea `633`.
- Runtime helper implementation, soft-float library work, expectations,
  unsupported markers, allowlists, timeouts, or accounting.

## Acceptance Criteria

- The refreshed probe classifies 16-byte local-memory residuals by type class
  and owner before implementation.
- Any implemented support requires explicit width, alignment, selected base,
  and payload authority and proves more than a named-case-only row.
- If the rows are primarily F128/long-double, the idea may close as a
  quarantine/policy classification without claiming ordinary-C consumer
  progress.

## Reviewer Reject Signals

- Reject using F128/long-double rows to steer ordinary non-F128 recovery
  without fresh breadth evidence.
- Reject treating all 16-byte accesses as the same semantic shape.
- Reject testcase-shaped handling for `src/20010605-2.c`,
  `src/20040208-1.c`, `src/ieee/inf-1.c`, or `src/complex-7.c`.
- Reject expectation, unsupported-marker, allowlist, timeout, runtime, or
  accounting changes as capability progress.
- Reject helper renames or diagnostic wording changes that leave the same
  unsupported-width behavior behind a new label.
