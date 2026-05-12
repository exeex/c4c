Status: Active
Source Idea Path: ideas/open/197_bir_backend_compatibility_string_retirement.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Convert Link-Visible Symbol Identity

# Current Packet

## Just Finished

Step 3 - Convert Link-Visible Symbol Identity reviewed the raw function-symbol
uses in `src/backend/bir/lir_to_bir/globals.cpp` across
`is_known_raw_function_symbol()`, `is_known_function_global_address()`,
`resolve_known_global_address()`, and `resolve_pointer_initializer_offsets()`.

The globals pointer-initializer/global-address raw lookup is now explicitly
fenced as a Step 3 no-id compatibility bridge. Its owner is the globals
pointer-initializer/global-address resolver in `globals.cpp`, its limitation is
that `FunctionSymbolSet` is populated only after module-boundary `LinkNameId`
resolution so metadata-rich missing ids cannot recover through raw initializer
spelling here, and its removal condition is LIR pointer initializers and
aggregate pointer fields carrying resolvable `LinkNameId` metadata.

No focused test was added because this packet only documents/fences existing
behavior. Existing pointer-initializer tests already cover raw/no-id
compatibility and metadata-rich missing-id fail-closed behavior for scalar and
aggregate pointer initializers.

## Suggested Next

Continue Step 3 by searching for any remaining semantic
`find_raw_symbol_link_name_id` or raw link-visible symbol fallback outside the
already fenced pointer-store address resolver, local-slot pointer-store
value/address, call-pointer argument, local-aggregate pointer-value alias,
declaration identity, direct-call, and globals pointer-initializer/global-address
paths.

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

## Proof

Delegated proof run:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`

Result: passed. `test_after.log` contains the build and backend CTest output;
CTest reported 100% tests passed, 0 failed out of 109 run, with 12 disabled
backend CLI trace/focus tests not run.
