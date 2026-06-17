# RV64 Prepared Emitter Context Interface Cleanup

## Goal

Tighten the internal interfaces introduced by the RV64 prepared emitter
decomposition so future RV64 capability can be added through owner-local APIs
instead of passing raw prepared lookup tables, name tables, block labels, and
instruction indices across every helper boundary.

This is a behavior-preserving cleanup idea. It should not add new RV64 runtime
cases or broaden backend capability while the interface cleanup is in progress.

## Why This Exists

Idea 301 successfully moved the prepared RV64 emitter out of the oversized
`src/backend/mir/riscv/codegen/emit.cpp` pile. The resulting layout is much
healthier:

- `emit.cpp` is now public compatibility glue plus legacy direct-LIR helpers;
- prepared frame, scalar, local-memory, call, edge-publication, function, and
  module emission have their own files;
- qemu-backed RV64 runtime coverage remained green;
- backend-wide validation preserved only the known
  `backend_riscv_prepared_edge_publication` baseline failure.

The split exposed the next layer of design debt: the new owner files still
share state through repeated long parameter lists and broad helper headers.
Common examples include:

- `PreparedNameTables` plus `PreparedFunctionLookups*` flowing through scalar,
  local-memory, call, and function helpers;
- repeated `BlockLabelId` plus instruction-index lookups at cross-owner
  boundaries;
- value-home and prepared-register queries living in the scalar header because
  several owners need them;
- stack/frame-slot offset helpers exposed broadly instead of through a small
  prepared-emission context;
- local-memory depending on scalar helpers and scalar depending back on
  local-memory stack load/store helpers.

That shape is acceptable immediately after a mechanical split, but it will
make the next RV64 feature additions drift toward another shared helper web if
left alone.

## In Scope

- Introduce a small prepared RV64 emission context/support surface that owns
  common prepared lookup operations used across owner files.
- Move value-home, prepared-register, prepared-pointer-register, and
  rematerializable-immediate queries out of feature-family headers when they
  are shared infrastructure rather than scalar lowering.
- Reduce repeated cross-owner parameter bundles such as
  `PreparedNameTables`, `PreparedFunctionLookups*`, block label, and
  instruction index where a context object or narrow lookup handle can carry
  them.
- Break avoidable include cycles or owner leakage between scalar and
  local-memory helpers.
- Keep owner files organized by backend responsibility, not by testcase.
- Preserve current fail-closed behavior for unsupported RV64 forms.
- Update build metadata for any new support files.

## Out of Scope

- Adding new RV64 runtime cases.
- Adding support for globals, indirect calls, varargs, stack arguments,
  aggregate ABI, dynamic stack, object emission, or any other new backend
  capability.
- Rewriting RV64 around the AArch64 machine-node architecture.
- Changing the qemu runtime test runner contract.
- Fixing the known `backend_riscv_prepared_edge_publication` baseline failure
  unless a separate idea explicitly owns that repair.
- Hiding unsupported behavior by weakening tests or expectations.

## Suggested Execution Order

1. Inventory the current prepared RV64 owner headers and mark which exported
   helpers are true owner APIs versus shared prepared-lookup utilities.
2. Add a small support header/source pair, for example
   `prepared_emit_context.{hpp,cpp}` or similarly named files, for shared
   prepared lookup/context helpers.
3. Move value-home/register/immediate lookup helpers into that support surface
   and update scalar, local-memory, and call owners to use it.
4. Move common current-location metadata into a narrow context or helper type
   if it reduces repeated parameter bundles without hiding ownership.
5. Revisit stack/frame-slot helper exposure and keep frame-specific operations
   under the frame/support owner rather than feature-family headers.
6. Run narrow RV64 runtime validation after each meaningful extraction.
7. Run focused RISC-V and backend-wide validation before closure.

If the cleanup discovers that a helper needs semantic redesign rather than
interface movement, stop and open a separate capability or architecture idea
instead of changing behavior under this cleanup.

## Acceptance Criteria

- `emit.cpp` remains thin and does not regain prepared lowering ownership.
- Prepared owner headers expose less shared infrastructure than after idea 301;
  scalar headers should not be the catch-all place for value-home and register
  lookup utilities.
- Cross-owner calls use a smaller, clearer context/support API instead of
  repeatedly passing raw prepared tables plus block/instruction coordinates.
- The scalar/local-memory dependency direction is cleaner, with stack load/store
  utilities either clearly owned by memory/frame support or factored as shared
  support.
- No new RV64 runtime case or backend capability is added under this idea.
- `ctest --test-dir build -R '^backend_rv64_runtime' --output-on-failure`
  passes after meaningful interface moves.
- `ctest --test-dir build -R 'backend_riscv|backend_codegen_route_riscv64' --output-on-failure`
  either passes or preserves only documented baseline failures.
- `ctest --test-dir build -R '^backend_' --output-on-failure` is run before
  closure and preserves the accepted baseline.

## Reviewer Reject Signals

- The cleanup changes emitted assembly behavior without a separate repair
  explanation.
- A new central support file becomes an unbounded "everything" pile.
- Headers remain broad enough that future scalar/call/memory capability would
  naturally be added through shared utility sprawl.
- The route weakens qemu runtime tests or accepts BIR/LLVM fallback.
- The diff is mostly testcase expectation churn rather than interface cleanup.

## Notes For The Agent

- Treat idea 301 as the starting layout. This idea should refine the boundary
  between those owner files, not undo the split.
- Prefer small mechanical moves with green runtime proof over a single large
  reshuffle.
- Keep helper names boring and ownership-oriented.
