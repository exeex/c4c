Status: Active
Source Idea Path: ideas/open/197_bir_backend_compatibility_string_retirement.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Closure Ledger And Broader Proof

# Current Packet

## Just Finished

Step 5 - Audit Backend-Prepared Route-Local Names is lifecycle-accepted based
on `review/step5_route_local_names_review.md`.

Acceptance result:
- Reviewer found no blocking findings and no testcase-overfit evidence.
- Reviewer judged the Step 5 route aligned with
  `ideas/open/197_bir_backend_compatibility_string_retirement.md` and the
  active runbook.
- Step 5 classified retained backend-prepared, route-debug, x86 route summary,
  module label, prepared query/dispatch/operand, and lowering-helper names as
  route-local, diagnostics/display, final assembler spelling, target-physical
  register spelling, interned-id display, or explicit no-id compatibility.
- Step 5 made no supported-path contract or test expectation changes.
- Step 5 proof was build plus backend CTest; 109 backend tests passed, with the
  disabled route-debug focus CLI tests still not run.

## Suggested Next

Step 6 - Closure Ledger And Broader Proof should produce the final
reviewer-auditable compatibility ledger in this file before closure.

Closure ledger work to do:
- Summarize deleted, converted, fenced, and intentionally retained string paths
  across text/string-constant identity, link-visible symbol identity,
  type/layout compatibility, route-local names, diagnostics, final spelling,
  and explicit raw/no-id compatibility.
- State whether new backend restart work may consume the BIR/prepared
  interface without adding rendered-name recovery fallbacks.
- Identify any retained no-id compatibility paths and their owner, limitation,
  and removal condition.
- Record the supervisor-selected broader validation command and result for the
  final compatibility retirement slice.
- Confirm focused proofs cover structured success, stale-id fail-closed
  behavior, and retained no-id compatibility where applicable.

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
- The x86 prepared dispatch/query fences added here are comment-only. Public
  rendered focus/entry names must keep resolving through the prepared name table
  before consumers touch prepared route state.
- The x86 lowering helper fences added here are comment-only. `frame_comment()`
  is display text, and `stack_mem()` renders final assembler syntax from an
  already-prepared frame offset; neither helper owns semantic identity or frame
  policy.
- Step 6 is a closure/proof ledger step, not an implementation expansion. If
  the ledger exposes a missing upstream carrier or separate backend restart
  requirement, record a separate open idea instead of widening this plan.
- Step 6 broader validation must not treat the Step 5 backend-only proof as
  full acceptance proof; the review noted disabled route-debug focus CLI tests.

## Proof

Delegated proof run:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`

Result: passed. `test_after.log` contains the build and backend CTest output;
CTest reported 100% tests passed, 0 tests failed out of 109 run, with 12
disabled backend CLI trace/focus tests not run.
