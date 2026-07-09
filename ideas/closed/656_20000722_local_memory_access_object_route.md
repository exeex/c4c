# 20000722 Local Memory Access Object Route

Status: Complete
Type: Investigation/Implementation
Parent: `ideas/open/648_rv64_call_arg_frame_slot_address_materialization.md`
Related:
- `ideas/open/648_rv64_call_arg_frame_slot_address_materialization.md`
- `ideas/closed/638_rv64_string_label_pointer_runtime_object_correctness.md`
- `ideas/closed/630_string_constant_local_memory_policy.md`
- `ideas/closed/618_runtime_mismatch_ownership_investigation.md`
Owning Layer: RV64 object-route local-memory access legality for the
representative `src/20000722-1.c` path
Queue Order: 56
Proof Surface: `src/20000722-1.c` object-route diagnostics plus focused
backend coverage for the first unsupported local-memory access owner

## Goal

Classify and repair, or explicitly split again, the fresh
`unsupported_local_memory_access` blocker that prevents the
`src/20000722-1.c` RV64 object route from reaching the call-argument
disassembly needed by idea 648.

## Why This Exists

Idea 648 added focused RV64 backend coverage for
`arg.source_selection=local_frame_address_materialization`. That coverage is
green without implementation changes and proves the text-route call-argument
consumer emits an `addi a0, sp, ...` frame-slot address setup rather than the
stale register-home copy.

Fresh representative object-route evidence for `src/20000722-1.c` no longer
reaches the historical `mv a0,s2` disassembly. It stops earlier with
`unsupported_local_memory_access`, outside the call-argument consumption
boundary. This idea owns that earlier blocker so idea 648 does not broaden into
local-memory policy or claim representative proof from stale evidence.

## In Scope

- Refresh the `src/20000722-1.c` RV64 object-route diagnostics and identify the
  exact local-memory access that fails with `unsupported_local_memory_access`.
- Locate the object-emission or lowering boundary that rejects that access.
- Add focused backend coverage for the first unsupported local-memory access
  owner before changing implementation behavior.
- Repair the narrow owner only if it is a legitimate already-prepared
  local-memory access that the RV64 object route should support.
- If the blocker belongs to a broader local-memory policy family, split a new
  source idea instead of absorbing that family here.

## Out Of Scope

- Reimplementing idea 648 call-argument frame-slot address materialization.
- Reopening broad string-constant local-memory admission or
  `StringConstantLabelPointer` policy.
- Generic ABI, stack-layout, branch/control-flow, or runtime rewrites.
- Source-file-specific handling for `src/20000722-1.c`, `%lv._clit_`, `foo`,
  `s2`, or `a0`.
- Expectation, unsupported-marker, allowlist, timeout, runtime-comparison, or
  pass/fail accounting changes.

## Acceptance Criteria

- The first `unsupported_local_memory_access` owner for the
  `src/20000722-1.c` object route is named with focused diagnostics.
- Focused coverage proves the accepted behavior for that owner, either as a
  supported object-route local-memory access or as a deliberate fail-closed
  rejection.
- If a narrow repair is valid, `src/20000722-1.c` advances past the
  `unsupported_local_memory_access` blocker.
- Any renewed call-argument materialization mismatch is handed back to idea 648
  rather than fixed through this local-memory route.
- No expectation, unsupported-marker, allowlist, timeout/accounting, or
  runtime-comparison change is claimed as capability progress.

## Completion Note

Closed after Step 4 repaired the narrow RV64 object-route local-memory blocker
for prepared compound-literal string-label pointer frame-slot stores. Focused
local-memory coverage passes, and the representative `src/20000722-1.c` object
route now emits an object instead of stopping at `unsupported_local_memory_access`.

The post-repair representative row reaches call-argument setup again. A fresh
asm snapshot still contains the stale `mv a0, s1` argument copy, so the
remaining owner is handed back to
`ideas/open/648_rv64_call_arg_frame_slot_address_materialization.md`.

## Reviewer Reject Signals

- Reject any route that changes idea 648 call-argument lowering while claiming
  to repair the local-memory object-route blocker.
- Reject filename-only or shape-only handling for `src/20000722-1.c`,
  `%lv._clit_`, `foo`, `s2`, or `a0`.
- Reject broad string-literal, local-memory, ABI, stack-layout, branch, or
  runtime rewrites without focused evidence for the first
  `unsupported_local_memory_access` owner.
- Reject expectation, unsupported-marker, allowlist, timeout/accounting,
  runtime-comparison, or pass/fail accounting changes as progress.
- Reject claiming idea 648 representative proof unless object/disassembly
  evidence actually reaches the call-argument setup after this blocker.
