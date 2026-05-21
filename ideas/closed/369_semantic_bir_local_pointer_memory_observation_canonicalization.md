# Semantic BIR Local/Pointer Memory Observation Canonicalization

Status: Closed
Created: 2026-05-21
Closed: 2026-05-21
Split From: ideas/open/295_backend_regex_failure_family_inventory.md

## Goal

Repair semantic-BIR local and pointer memory observation canonicalization so
backend-route tests see the intended local stores, pointer-parameter accesses,
and dynamic aggregate/member lanes instead of computed-pointer detours or
string-literal address substitutions.

## Why This Exists

The Step 2 backend-regex classification in `todo.md` found seven local
backend-route residuals that fail before runtime as
`BACKEND_ROUTE_SNIPPET_MISSING`. Their first bad boundary is the semantic-BIR
observation surface, not AArch64 code generation, x86 machine lowering,
runner behavior, or CTest registration.

Representative evidence:

- `backend_codegen_route_x86_64_pointer_param_loaded_char_deref_observe_semantic_bir`
  expects `bir.store_local %lv.p, ptr %t0`, while the actual dump stores
  `ptr @.str0` after loading `%lv.p` with `addr .str0`.
- `backend_codegen_route_x86_64_nested_pointer_param_dynamic_member_array_load_observe_semantic_bir`
  expects an observed element load from the member lane, while the actual dump
  computes a dynamic byte offset and loads through the computed pointer.
- The local dynamic member-array load/store, direct local dynamic
  member-array load/store, and packed local member-offset observers likewise
  expect local aggregate/member lane materialization but currently see
  computed pointer load/store paths.

This owner is separate from the parked pointer-derived string-load idea 356:
that idea repaired dynamic byte loads for `00173`, while the current failures
are local backend-route observation mismatches for local/pointer canonical
semantic-BIR shape.

## In Scope

- Localize the semantic-BIR canonicalization point that chooses between local
  aggregate/member lane operations and computed-pointer load/store operations.
- Repair string-literal pointer local materialization enough that pointer
  parameter/local observers preserve the expected local store/load shape.
- Repair dynamic indexed local aggregate/member lane materialization for
  pointer parameters, direct locals, and packed-member offsets.
- Add or update focused semantic/backend-route coverage only to prove the
  semantic-BIR shape changed because the lowering rule is correct.
- Prove the seven classified backend-route residuals advance or pass without
  changing expectations as a substitute for capability repair.

## Out Of Scope

- AArch64 runtime mismatches, generated assembly, register allocation,
  machine-printer behavior, ABI call-boundary transport, libc calls,
  initializer lowering, unsigned enum bitfield lowering, switch/fallthrough
  lowering, timeout handling, runner policy, CTest registration, proof-log
  policy, unsupported classifications, or external c-testsuite expectations.
- Reopening parked idea 356 unless fresh evidence shows dynamic
  pointer-derived byte loads again collapse to fixed global-byte loads.
- Broad local backend-route expectation cleanup unrelated to the seven
  classified semantic-BIR observation failures.
- Filename-only, test-name-only, or emitted-text-only fixes that do not repair
  the semantic memory representation rule.

## Acceptance Criteria

- The first bad boundary is localized to semantic-BIR local/pointer memory
  canonicalization, with evidence for the affected pointer-local and
  dynamic aggregate/member lane shapes.
- Focused semantic/backend-route coverage fails before the repair and passes
  after it for string-literal pointer local materialization and dynamic local
  aggregate/member lane access.
- The seven Step 2 residuals in the semantic-BIR local/pointer memory
  observation bucket advance or pass without expectation, runner, timeout,
  CTest-registration, proof-log, or unsupported-classification changes.
- Nearby semantic pointer-derived load coverage, including the completed
  idea 356/365/366 chain, remains stable or is explicitly reclassified by a
  new first bad fact.

## Completion Notes

Closed after Step 4 proved the semantic-BIR local/pointer memory observation
repairs with the delegated focused backend subset and nearby regression
boundaries. The seven idea 369 backend-route targets passed in the focused
proof recorded in `todo.md`.

Close retry used the canonical full-suite regression logs:

- `test_before.log`: 3333 passed, 41 failed, 3374 total.
- `test_after.log`: 3340 passed, 34 failed, 3374 total.
- `c4c-regression-guard` result: PASS, with zero new failures and seven
  resolved failures.

No expectation, runner, timeout, CTest-registration, proof-log, unsupported
classification, or out-of-scope backend target-machine policy changes were
part of the lifecycle close decision.

## Reviewer Reject Signals

Reject the route if it:

- special-cases one backend-route test name, `@.str0`, one temporary, one
  local variable, one packed offset, one member index, or one emitted dump
  snippet instead of repairing a general semantic-BIR memory representation
  rule;
- claims progress by weakening route expectations, changing unsupported
  classifications, runner behavior, timeout policy, proof-log policy, or
  CTest registration;
- only renames helpers, reshapes dumps, or rewrites snippets while the same
  local/pointer memory operation still lowers through the wrong computed
  pointer or string-literal substitution shape;
- broadens into AArch64 runtime, ABI, libc, initializer, bitfield, switch,
  aggregate writeback, timeout, or external c-testsuite work without a fresh
  lifecycle split;
- reopens parked pointer-derived string-load idea 356 or downstream string
  literal publication ideas from counts alone instead of showing the exact old
  first bad fact has returned.
