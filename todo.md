Status: Active
Source Idea Path: ideas/open/197_bir_backend_compatibility_string_retirement.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Fence Type And Aggregate Layout Compatibility

# Current Packet

## Just Finished

Step 4 - Fence Type And Aggregate Layout Compatibility reviewed and classified
the remaining owned aggregate/byval layout call sites in `aggregate.cpp` and
`memory/local_slots.cpp`.

`aggregate.cpp` now explicitly fences the shared rendered-text
`lower_byval_aggregate_layout()` entrypoint, the legacy byval parameter
collection branch that lacks `StructNameId` metadata, and local aggregate
copy-size validation from `LocalAggregateSlots` rendered type text. The existing
metadata-bearing byval parameter branch still uses the `LirTypeRef` /
`StructNameId` lookup path and fails closed there.

`memory/local_slots.cpp` now explicitly fences aggregate store and aggregate
load lowering from `LirStoreOp::type_str` / `LirLoadOp::type_str`. Both paths
remain compatibility bridges until aggregate memory ops carry structured type
identity.

Final Step 4 inventory for the owned files:
`rg -n "lower_byval_aggregate_layout\\(|lookup_backend_aggregate_type_layout|Step 4 no-id compatibility bridge" src/backend/bir/lir_to_bir/aggregate.cpp src/backend/bir/lir_to_bir/memory/local_slots.cpp`
shows each remaining raw-text aggregate/byval layout call in these files is
either behind a Step 4 no-id compatibility fence or on the existing structured
`LirTypeRef` / `StructNameId` fail-closed path.

No test expectations were changed; `tests/backend/backend_lir_to_bir_notes_test.cpp`
was not modified.

## Suggested Next

Ask the plan owner or reviewer to decide whether Step 4 is complete enough to
advance to the next plan step, using the existing Step 4 review artifact plus
this final owned-file inventory.

## Watchouts

- Do not start AArch64 backend implementation work.
- Do not downgrade supported tests or replace semantic fixes with expectation
  rewrites.
- Do not treat printer, assembler, linker, diagnostic, debug-focus, or
  route-local spelling as semantic identity.
- If parser, Sema, HIR, or LIR source intent blocks this work, record a
  separate open idea instead of expanding this plan.
- Step 2 intentionally did not add an upstream `TextId` field to
  `LirStringConst`; the structured carrier is created at the LIR-to-BIR
  boundary in the BIR module text table. If a later slice needs producer-owned
  text-pool ids, record that as a separate upstream carrier initiative instead
  of widening this plan.
- x86 `module/data.cpp` raw same-module global lookup is currently final
  assembler/output spelling territory. Do not convert that before the upstream
  BIR/prepared link-name carriers are fenced.
- The direct-call bridge review is complete, and the local direct pointer-store,
  call-pointer argument, and local-aggregate pointer-value alias bridges are
  now fenced.
- The `calling.cpp` declaration identity and direct-call fallback review is
  complete. Those fallbacks are no-id compatibility only; do not bypass the
  module-boundary `LinkNameId` checks or `FunctionSymbolSet` membership when
  changing them later.
- The local pointer-store value and address-tracking fences rely on
  `FunctionSymbolSet` construction failing closed for metadata-bearing
  functions or extern declarations whose `LinkNameId` spelling cannot be
  resolved; do not bypass that module-boundary check in a later pointer bridge.
- The call-pointer argument and local-aggregate pointer-value alias fences have
  the same boundary assumption: they are only for no-id imported function
  pointer operands, not a way to recover corrupted metadata-rich function
  identity.
- The `resolve_pointer_store_address()` fence has the same boundary assumption
  for pointer-store address operands: it is no-id compatibility only and must
  not become a raw-spelling recovery path for corrupted metadata-rich identity.
- The `globals.cpp` pointer-initializer/global-address fence has the same
  boundary assumption: scalar and aggregate pointer initializers with present
  function `LinkNameId` metadata must pass `FunctionSymbolSet` membership and
  must not recover through raw initializer spelling.
- The central `TypeDeclMap` layout fallback is retained only for no-id legacy
  declarations. Metadata-bearing type refs must stay on
  `lookup_backend_aggregate_type_ref_layout_result()` so stale `StructNameId`
  text and parity mismatches fail closed instead of recovering through rendered
  type spelling.
- The local GEP/addressing wrapper fence is comment-only and deliberately does
  not tighten `resolve_aggregate_*` public raw-text helpers; their signatures do
  not yet carry `LirTypeRef` metadata.
- The local-slot scalar byte-offset fence is comment-only and deliberately does
  not change dynamic local aggregate array behavior; those helpers still only
  carry rendered element type text.
- The aggregate.cpp selected-layout fence is comment-only and deliberately does
  not change local aggregate slot declaration, leaf discovery, or byval copy
  behavior; metadata-bearing byval params already use the `LirTypeRef` lookup
  path and should keep failing closed there.
- The local pointer-store materialization conversion is behavior-preserving:
  it still uses the same structured layout table when available, but the direct
  raw aggregate layout call is now behind the local scalar byte-offset wrapper
  fence.
- The provenance scalar-subobject fence is comment-only. It does not add
  structured layout authority because the current address carriers still lack
  `LirTypeRef` / `StructNameId` metadata.
- The `addressing.cpp` conversion is behavior-preserving: the direct GEP and
  pointer-value layout lookups now enter the same fenced helper used by the
  projection helpers, but the underlying public raw-text helpers are not
  tightened until their signatures carry structured type identity.
- `local_gep.cpp` needed no code change in this packet; its wrapper already
  owns the Step 4 no-id compatibility bridge and structured-layout mismatch
  guard.
- The global initializer/global layout/call ABI fences are comment-only and
  deliberately preserve current compatibility behavior. Structured global
  layout and enforced AArch64 signature aggregate layout paths still fail
  closed when `LirTypeRef` / `StructNameId` metadata is missing or mismatched.
- The `memory/intrinsics.cpp` wrapper fence is comment-only. It deliberately
  preserves local memset/memcpy compatibility for aggregate and pointer-derived
  leaf views that still carry only rendered type text.
- The `calling.cpp` byval/aggregate fences are comment-only. They deliberately
  preserve legacy call, local aggregate alias, and variadic runtime aggregate
  behavior while keeping existing `LirTypeRef` / `StructNameId` call-argument
  checks fail-closed.
- The `cfg.cpp` aggregate PHI fence is comment-only. `LirPhiOp::type_str` is a
  `LirTypeRef`, but `PhiLoweringPlan` currently records only rendered text; do
  not treat aggregate PHI layout as structured until the plan carries the ID.
- The `aggregate.cpp` and `memory/local_slots.cpp` fences are comment-only and
  behavior-preserving. They classify existing no-id compatibility bridges; they
  do not add structured type authority to local aggregate slots or aggregate
  memory load/store ops.
- `tests/backend/backend_lir_to_bir_notes_test.cpp` was reviewed only through
  the delegated backend proof; no expectation changes were needed or made.

## Proof

Delegated proof run:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`

Result: passed. `test_after.log` contains the build and backend CTest output;
CTest reported 100% tests passed, 0 tests failed out of 109 run, with 12
disabled backend CLI trace/focus tests not run.
