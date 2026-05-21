# AArch64 Indexed Aggregate Snapshot Writeback Regression

Status: Open
Created: 2026-05-21
Split From: ideas/open/295_backend_regex_failure_family_inventory.md
Follow-up To: ideas/closed/348_aarch64_indexed_aggregate_address_writeback.md

## Goal

Repair the current AArch64 dynamic indexed aggregate address/writeback
regression where selected local or global aggregate elements are lowered
through fixed snapshots, high stack temporaries, or stale aggregate copies
instead of the source-selected element address.

## Why This Exists

The post-370 backend inventory classified the current backend-regex surface as
356 selected tests, 344 passed, and 12 failed. Local backend/unit/route/MIR,
BIR, CLI, runtime, and smoke tests are clean; all residuals are external
`c_testsuite_aarch64_backend_*`.

The strongest current bucket is dynamic indexed local/global aggregate
address/writeback, represented by `00157` and `00176`. This overlaps closed
idea 348, but the current evidence is fresh generated assembly after that
closure rather than a count-only reopen:

- `00157` stores `Count * Count` into `Array[Count-1]`, but generated AArch64
  snapshots the local array through fixed stack slots and later reads high
  uninitialized slots such as `[sp,#92]`, `[sp,#100]`, and nearby offsets.
- `00176` quicksort/swap again expands global `array[...]` through large
  select/snapshot chains and stack temporaries, producing an unsorted array
  instead of writing back through the selected global elements.

Closed idea 348 repaired prior selected-address/writeback representatives,
including a previous `00176` first bad fact. This follow-up exists because the
current first bad facts show the same semantic owner boundary is again active
under a snapshot/writeback shape that must be repaired or explicitly narrowed
without mutating the closed archive.

## In Scope

- Localize how dynamic indexed local and global aggregate element addresses
  are represented through BIR/prepared/MIR/AArch64 lowering for `00157` and
  `00176`.
- Identify where selected element address authority is replaced by fixed
  stack snapshots, uninitialized high stack slots, stale aggregate copies, or
  non-writeback temporaries.
- Repair the general AArch64 selected element address/writeback path for
  dynamic local arrays and global array swaps.
- Add focused backend coverage for the current snapshot/writeback failure
  shape, independent of only `00157` or `00176`.
- Prove both representative cases advance past the current first bad facts or
  are reclassified by a new owner boundary.

## Out Of Scope

- Reopening closed idea 348's archive or claiming closure invalidation from
  failing counts alone.
- Scalar FP constant/expression materialization, conditional/switch arm
  materialization, external/libc call return publication, pointer/subobject
  address publication, global `sizeof` metadata, complex initializer layout,
  enum bit-field storage/load layout, or timeout quarantine.
- Variadic/byval/HFA ABI, semantic BIR route-observation failures,
  pointer-derived string loads, local scalar writeback sizing, call-boundary
  argument preservation, or runner/proof-log policy.
- Fixing only `00157`, `00176`, one array name, one global symbol, one stack
  offset, one register, or one emitted instruction sequence.

## Acceptance Criteria

- The current first bad fact is localized to a concrete selected aggregate
  address, snapshot, stack-home, emitted load/store, or writeback handoff
  boundary.
- Focused backend coverage guards dynamic indexed local/global aggregate
  element writeback without depending only on the external representatives.
- `c_testsuite_aarch64_backend_src_00157_c` and
  `c_testsuite_aarch64_backend_src_00176_c` advance past their current
  snapshot/writeback first bad facts or pass.
- If either representative remains failing, `todo.md` records the new first
  bad fact and whether it stays in this owner or needs a separate lifecycle
  split.
- The proof subset preserves the prior selected-address/writeback repairs
  relevant to closed idea 348 and introduces no new backend-regex failures in
  the supervisor-selected guard scope.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00157`, `00176`, one local/global array, one stack offset,
  one global symbol, one index expression, one register, or one emitted
  load/store sequence instead of repairing selected aggregate
  address/writeback generally;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, CTest registration, or external test contracts;
- claims progress through helper renames, emitted-text reshuffling, or
  classification-only notes while generated AArch64 still reads or writes
  selected elements through stale snapshots or uninitialized stack homes;
- reopens or rewrites closed idea 348 instead of using fresh evidence to drive
  this follow-up route;
- folds scalar FP, conditional/switch, external-call return, pointer/subobject,
  global metadata, complex initializer, bit-field, timeout, or ABI work into
  this route without a fresh first-bad-fact handoff;
- proves only one representative while the nearby current indexed aggregate
  shape remains unexamined.
