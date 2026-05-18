# AArch64 C-Testsuite Failure Family Inventory

Status: Open
Created: 2026-05-18

## Intent

Use the current AArch64 backend c-testsuite scan as an umbrella inventory. The
work should inspect failing cases by semantic family, then create focused
repair ideas and switch to them before implementation starts.

## Current Evidence

The active AArch64 backend c-testsuite route has 212 registered cases after
runtime-hang quarantine removed:

```text
00021.c 00025.c 00033.c 00040.c 00078.c 00080.c 00083.c 00084.c
```

The latest useful broad scan command was:

```bash
ctest --test-dir build-aarch64-scan -L '^aarch64_backend$' -j 8 --timeout 5 --output-on-failure
```

The rough classification was:

```text
212 total
 46 PASS
 49 FRONTEND_FAIL
 87 RUNTIME_NONZERO
  7 RUNTIME_MISMATCH
 23 TIMEOUT
  0 BACKEND_FAIL
```

Transient logs from that scan, if still present:

```text
/tmp/c4c_aarch64_backend_scan_212.log
/tmp/c4c_aarch64_backend_scan_212.classified
```

If those logs are gone or stale, rerun the scan with an explicit timeout and
check for stale generated-runtime processes afterward.

## In Scope

- Classify the 212-case AArch64 backend route by failure family.
- Inspect representative files from each bucket instead of fixing one named
  testcase directly.
- Separate frontend-owned failures from backend/runtime-owned failures.
- Split focused `ideas/open/*.md` files when a repair direction is semantic and
  tractable.
- Switch lifecycle state to a focused idea before coding.

## Out of Scope

- Implementing repairs inside this umbrella idea.
- Asking an executor to run broad runtime scans without timeout and stale
  process cleanup.
- Treating timeout/hang cases as ordinary executor work before safer families
  are handled.
- Weakening c-testsuite expected outputs, allowlists, unsupported
  classifications, CTest contracts, or runner behavior to improve counts.
- Matching c-testsuite filenames or exact emitted instruction text instead of
  identifying the semantic owner.

## Completion Criteria

- A current inventory exists in `todo.md`.
- At least one focused repair idea is created from a semantic failure family,
  or `todo.md` explains why no family is ready to split.
- Timeout/hang cases are deferred, quarantined, or split into a safe
  hang-specific idea.
- The active lifecycle state switches from this umbrella idea to a focused
  repair idea before implementation work begins.

## Deactivation Note

2026-05-18: Inventory runbook completed its split point and was deactivated as
the active implementation target. The first focused repair idea created from
the inventory is
`ideas/open/285_aarch64_backend_nonleaf_call_frame_lr_preservation.md`.

2026-05-18: The non-leaf LR preservation idea completed and moved to
`ideas/closed/285_aarch64_backend_nonleaf_call_frame_lr_preservation.md`.
The next focused semantic follow-on is
`ideas/open/286_aarch64_scalar_call_value_semantics.md`.

2026-05-18: The scalar call-value semantics idea completed and moved to
`ideas/closed/286_aarch64_scalar_call_value_semantics.md`. The refreshed
inventory then selected AArch64 backend string/global address materialization
plus external-call argument setup as the next focused semantic repair family.
The active lifecycle state switched to
`ideas/open/287_aarch64_string_global_address_external_call_lowering.md`.

2026-05-18: The string/global address external-call lowering idea completed
and moved to
`ideas/closed/287_aarch64_string_global_address_external_call_lowering.md`.
The next refreshed inventory selected AArch64 stack-frame/SP 16-byte
alignment as the next focused semantic repair family. The active lifecycle
state switched to
`ideas/open/288_aarch64_stack_frame_sp_alignment.md`.

2026-05-18: The stack-frame/SP alignment idea completed and moved to
`ideas/closed/288_aarch64_stack_frame_sp_alignment.md`. Wider former
bus-error sampling left four aligned-frame bus-error cases with bad or missing
function-pointer values before indirect `blr`; the active lifecycle state
switched to
`ideas/open/289_aarch64_function_pointer_indirect_call_values.md`.

Durable inventory findings to preserve:

- The 212-case AArch64 backend c-testsuite scan classified as 46 `PASS`, 49
  `FRONTEND_FAIL`, 87 `RUNTIME_NONZERO`, 7 `RUNTIME_MISMATCH`, and 23
  `TIMEOUT`, with no `BACKEND_FAIL` bucket.
- The timeout family should not be treated as ordinary runtime mismatch work.
  All 23 timeout cases had generated assembly containing at least one `bl`
  call, and inspected generated functions did not save/restore `x30` before
  `ret`.
- Initial timeout probes are `00100.c`, `00116.c`, and `00121.c` because they
  avoid stdio and isolate non-leaf function calls returning through a
  clobbered link register.
- Compound timeout cases should remain quarantined until the non-leaf
  link-register owner no longer masks their behavior. Secondary risks include
  string-literal/variadic calls, loop predicates, short-circuit control flow,
  local stack slots, aggregates/pointers, static globals, and goto behavior.
- After LR preservation, the original 23 timeout cases reclassified to two
  passes (`00100.c`, `00121.c`), one remaining timeout (`00132.c`), four
  `RUNTIME_NONZERO` cases (`00116.c`, `00175.c`, `00196.c`, `00199.c`), and
  sixteen `RUNTIME_MISMATCH` cases. The remaining owners are semantic backend
  families, not the old non-leaf LR preservation failure.
- After scalar call-value repair, the next selected family is represented by
  `src/00125.c`, `src/00131.c`, `src/00154.c`, `src/00161.c`, and
  `src/00211.c`. These cases expose missing or incorrect materialization of
  string-literal/global/static data addresses and address-valued arguments for
  direct external calls such as `printf`/stdio. `src/00132.c` overlaps this
  family but remains timeout-sensitive and compounded by loop/local-store
  behavior, so it should not be used as first proof for the focused route.
- After string/global address external-call lowering, the refreshed scan
  classified 212 registered AArch64 backend cases as 80 passed, 129 failed
  non-timeout, and 3 timed out. The best next focused family is a 28-case
  `Bus error` runtime cluster whose minimal representative `src/00004.c`
  emits a 24-byte stack frame (`sub sp, sp, #24`). That makes stack-frame/SP
  16-byte alignment the first owner to repair before treating nearby
  pointer/local/array failures as broader pointer-semantics issues.
- After stack-frame/SP alignment, the nearby local pointer/array subset
  `src/00004.c`, `src/00005.c`, `src/00013.c`, `src/00014.c`,
  `src/00015.c`, and `src/00016.c` passed. Wider former bus-error sampling
  selected 28 cases: 17 passed, 11 failed, and 0 timed out. Remaining bus
  errors `src/00087.c`, `src/00089.c`, `src/00124.c`, and `src/00210.c` had
  aligned frames and an indirect-call/function-pointer value failure shape, so
  they were split from the SP/frame-alignment owner.

## Reviewer Reject Signals

Reject the route if it:

- claims progress by fixing one named c-testsuite case without identifying the
  broader owner;
- expands this umbrella into implementation work;
- mixes frontend and backend failures without a clear boundary;
- downgrades expectations, allowlists, unsupported classifications, or CTest
  behavior to change pass counts;
- runs broad runtime scans without timeout and stale-process cleanup;
- forgets that timeout/hang work is late-stage and environment-sensitive.
