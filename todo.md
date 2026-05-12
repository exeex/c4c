Status: Active
Source Idea Path: ideas/open/197_bir_backend_compatibility_string_retirement.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Audit Backend-Prepared Route-Local Names

# Current Packet

## Just Finished

Step 5 - Audit Backend-Prepared Route-Local Names continued with a
target-local x86 route summary/trace packet.

Audit result:
- `src/backend/mir/x86/debug/debug.cpp` now classifies grouped route summaries
  and traces as debug display over prepared/prealloc authority. Target register
  spellings remain target-physical names, prepared value names remain
  interned-id display, and focus strings remain public dump filters over
  rendered text.
- `src/backend/mir/x86/module/module.cpp` now classifies x86 function-local
  labels as final assembly spellings derived after prepared block-label
  validation, narrow register spellings as target-physical assembly names, and
  grouped authority comments as diagnostics only.
- The `module.cpp` BIR block-label raw-text fallback is documented as legacy
  no-id compatibility only; metadata-bearing blocks still match through the
  structured BIR label id spelling.
- `tests/backend/backend_prepare_frame_stack_call_contract_test.cpp` and
  `tests/backend/backend_x86_route_debug_test.cpp` were audited as expectation
  coverage for those x86 route/debug surfaces. No behavior or expectation
  change was needed or made.

## Suggested Next

Continue Step 5 with the next backend-prepared naming packet: audit any
remaining backend-prepared route-local names outside the already covered
prealloc, prepared-printer, backend focus, and x86 summary/trace surfaces.

## Watchouts

- Do not start AArch64 backend implementation work.
- Do not downgrade supported tests or replace semantic fixes with expectation
  rewrites.
- Do not treat printer, assembler, linker, diagnostic, debug-focus, or
  route-local spelling as semantic identity.
- If parser, Sema, HIR, or LIR source intent blocks this work, record a
  separate open idea instead of expanding this plan.
- Step 5 should not rewrite backend-prepared or MIR interfaces broadly. Rename
  or comment retained route-local spellings only where that clarifies ownership
  for backend restart consumers.
- Step 4 is lifecycle-accepted, but the broader/full validation remains a Step 6
  closure requirement because central layout resolver and shared backend
  wrapper surfaces changed during Step 4.
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
- The Step 5 fences added here are comment-only and intentionally do not broaden
  backend-prepared or MIR interfaces. The block-index bridge remains until
  prepared control-flow consumers carry a structured BIR block handle/index from
  the producer.
- The route-debug focus fences added here are comment-only and intentionally do
  not change public focus behavior. Focus function/block/value strings are CLI
  dump selectors over rendered text; prepared-side trimming should keep using
  interned IDs where those IDs exist.
- The prepared-printer legacy `source_symbol_name` fallback remains display
  only. Do not treat it as a replacement for `LinkNameId` on metadata-bearing
  call plans.
- The x86 route summary/trace fences added here are comment-only. They
  deliberately preserve public route-debug output while classifying retained
  names as final asm spelling, target-physical register spelling, interned-id
  display, or route-local debug filtering.
- The x86 `module.cpp` block-label fallback remains no-id compatibility only.
  Do not use it to recover stale or mismatched metadata-bearing block identity.

## Proof

Delegated proof run:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`

Result: passed. `test_after.log` contains the build and backend CTest output;
CTest reported 100% tests passed, 0 tests failed out of 109 run, with 12
disabled backend CLI trace/focus tests not run.
