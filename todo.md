Status: Active
Source Idea Path: ideas/open/197_bir_backend_compatibility_string_retirement.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Fence Type And Aggregate Layout Compatibility

# Current Packet

## Just Finished

Step 4 - Fence Type And Aggregate Layout Compatibility reviewed the remaining
owned memory direct aggregate-layout raw-text callers, with focus on local
pointer-store address materialization.

The local aggregate pointer-store materialization branch in `local_slots.cpp`
now routes through the Step 4 fenced local scalar byte-offset layout wrapper
instead of calling `lookup_backend_aggregate_type_layout()` directly with
rendered local aggregate type text. The retained raw rendered-type-text owner
is the local-slot scalar byte-offset helper family; its limitation remains that
local aggregate and dynamic aggregate array state carry rendered type spelling
but not `LirTypeRef` / `StructNameId` metadata. The removal condition is those
memory address/aggregate carriers holding structured type refs.

The provenance scalar-subobject checker is also fenced as a Step 4 no-id
compatibility bridge. Its owner is provenance addressability validation for
`GlobalInfo`, `LocalSlotAddress`, and `PointerAddress` state; its limitation is
that those address carriers only pass rendered aggregate type text into scalar
layout fact lookup. The removal condition is provenance address state carrying
structured type refs alongside route-local spelling.

No test edits were needed because behavior did not change. The owned memory
files no longer contain a direct `lookup_backend_aggregate_type_layout()` call.

## Suggested Next

Continue Step 4 by reviewing adjacent non-owned memory aggregate layout callers
in `memory/addressing.cpp` and `memory/local_gep.cpp`. Convert callers to an
existing fenced local wrapper where the carrier still lacks structured type
metadata, or fail closed if a `LirTypeRef` / `StructNameId` carrier is already
available there.

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

## Proof

Delegated proof run:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`

Result: passed. `test_after.log` contains the build and backend CTest output;
CTest reported 100% tests passed, 0 failed out of 109 run, with 12 disabled
backend CLI trace/focus tests not run.
