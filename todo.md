Status: Active
Source Idea Path: ideas/open/197_bir_backend_compatibility_string_retirement.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Closure Ledger And Broader Proof

# Current Packet

## Just Finished

Step 6 - Closure Ledger And Broader Proof is complete.

Closure ledger:
- Text/string-constant identity: `bir::StringConstant` now carries
  `name_id`, metadata-rich LIR-to-BIR string-pool lowering materializes BIR
  constants through the module text table, and route-local string pointer
  aliases carry `TextId` payloads instead of treating raw spelling as identity.
  `StringConstant::name` is retained as display/raw-only compatibility and
  final output spelling. Step 2 intentionally did not add an upstream
  `TextId` field to `LirStringConst`; the structured carrier is created at the
  LIR-to-BIR boundary. Producer-owned text-pool ids, if later needed, belong in
  a separate upstream carrier idea.
- Link-visible symbol identity: metadata-rich globals, functions, extern
  declarations, direct calls, pointer initializers, pointer-store bridges, call
  pointer arguments, local aggregate pointer aliases, provenance/address
  tracking, and global-address initializers now use `LinkNameId` /
  `FunctionSymbolSet` authority when metadata is present. Present-but-unresolved
  `LinkNameId` metadata fails closed. Raw spelling lookup is retained only as
  named no-id import compatibility after the module-boundary structured checks.
- Type/layout compatibility: metadata-bearing aggregate refs stay on
  structured lookup paths such as `lookup_backend_aggregate_type_ref_layout_result`
  and fail closed on stale `StructNameId`, missing id, or parity mismatch.
  `TypeDeclMap`, selected raw layout helpers, local aggregate slot paths,
  call ABI raw signature branches, aggregate PHI layout, and memory intrinsic
  leaf views are fenced as no-id/rendered-text compatibility where the current
  carriers do not yet include `LirTypeRef` / `StructNameId` authority.
- Route-local names: backend-prepared value homes, move bundles, block indices,
  route-debug focus selectors, prepared-printer fallback names, x86 debug route
  summaries, module labels, prepared dispatch/query/operand names, and lowering
  helper spellings are classified as route-local state, diagnostics/display,
  final assembler spelling, target physical register spelling, or explicit
  no-id compatibility. Step 5 was comment-only and did not broaden backend or
  MIR interfaces.
- Diagnostics/final spelling: printer text, diagnostic notes, route-debug CLI
  filters, assembler labels/registers, linker/output names, `frame_comment()`,
  and `stack_mem()` remain valid rendered spelling consumers. They are not
  semantic identity and must not be used to recover metadata-rich lookup misses.
- Deleted/converted/fenced/retained summary: the plan converted text-pool
  string identity to BIR `TextId`, converted covered link-visible identity to
  `LinkNameId`, routed covered aggregate layout through structured lookup,
  fenced unavoidable rendered/raw helpers with owner/limitation/removal notes,
  and intentionally retained printer, assembler, linker, diagnostics, debug,
  route-local, and no-id import compatibility strings.

Backend restart consumption decision:
- New backend restart work may consume the post-retirement BIR/prepared
  interface without adding rendered-name recovery fallbacks for covered
  metadata-rich text, link-visible symbol, and type/layout identity. If a
  consumer lacks the needed structured carrier, it should either stay behind the
  existing named no-id compatibility boundary or record a separate upstream
  carrier initiative; it should not recover stale or missing metadata by
  matching rendered text.

Retained no-id compatibility owners, limitations, and removal conditions:
- `StringConstant::name`: owned by BIR/LIR-to-BIR string-pool compatibility and
  final output emission. Limitation: display/raw-only compatibility when no
  structured text identity exists. Removal condition: upstream LIR string-pool
  constants carry producer-owned `TextId` identity and all consumers use it.
- Raw function/global symbol lookup bridges: owned by LIR-to-BIR module
  import compatibility. Limitation: no-id payloads only after
  `LinkNameId`/`FunctionSymbolSet` checks; metadata-bearing misses must fail
  closed. Removal condition: all imported globals/functions/pointer operands
  carry resolvable `LinkNameId` metadata.
- `TypeDeclMap` and raw aggregate layout helpers: owned by LIR-to-BIR
  type/layout compatibility. Limitation: no-id legacy declarations, local
  aggregate slots, selected call ABI branches, aggregate PHI plans, and memory
  intrinsic leaf views whose carriers still expose rendered type text only.
  Removal condition: those producers carry `LirTypeRef` / `StructNameId`
  metadata through the relevant helper signatures.
- Backend-prepared route-local block/value/focus fallbacks: owned by prealloc,
  backend debug/focus, prepared-printer, and x86 route/debug/module surfaces.
  Limitation: route-local lookup, public dump filtering, display, final asm
  spelling, or no-id block-label compatibility only. Removal condition:
  prepared control-flow/debug consumers carry structured BIR block/value
  handles through the producer boundary.

Focused proof coverage:
- Structured success and stale-id fail-closed behavior are covered for BIR
  string constants, unresolved global/function/extern `LinkNameId`, direct call
  and pointer-symbol identity, structured global/type layout lookup, byval and
  aggregate layout mismatch, and retained no-id layout compatibility.
- Retained no-id compatibility is covered where executable behavior exists,
  including raw-only string pointer lowering, no-id global/function symbol
  bridges, and legacy/no-id `TypeDeclMap` layout fallback. Step 5 route-local
  audit was review-accepted as comment-only classification, with no test or
  supported-contract changes.

## Suggested Next

Supervisor should route to plan-owner for lifecycle closure review of
`ideas/open/197_bir_backend_compatibility_string_retirement.md`, using this
ledger plus `test_before.log` / `test_after.log` as the closure evidence.

## Watchouts

- Do not start AArch64 backend implementation work.
- Do not downgrade supported tests or replace semantic fixes with expectation
  rewrites.
- Do not treat printer, assembler, linker, diagnostic, debug-focus, or
  route-local spelling as semantic identity.
- If parser, Sema, HIR, or LIR source intent blocks follow-up backend restart
  work, record a separate open idea instead of expanding this plan.
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

## Proof

Delegated proof run:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure' > test_after.log 2>&1`

Result: passed. `test_after.log` contains the build and full-suite CTest
output. CTest reported 100% tests passed, 0 tests failed out of 3137 run, with
12 disabled backend CLI trace/focus tests not run.

Matching baseline: supervisor-provided `test_before.log` is the same full-suite
command shape and reported 100% tests passed, 0 tests failed out of 3137 run,
with the same 12 disabled backend CLI trace/focus tests not run.
