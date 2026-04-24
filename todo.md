Status: Active
Source Idea Path: ideas/open/93_hir_agent_index_header_hierarchy.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Clarify Inspection Support

# Current Packet

## Just Finished

Step 6 - Clarify Inspection Support separated full HIR dump inspection from the
core HIR public facade include path.

- Added `src/frontend/hir/impl/inspect/inspect.hpp` as the private inspection
  index for implementation/debug internals.
- Removed `hir_printer.hpp` from `hir.hpp`, leaving `format_summary(...)`
  public through the core facade while requiring `format_hir(...)` callers to
  include the printer/inspection boundary explicitly.
- Retargeted `c4cll.cpp` to include `hir_printer.hpp` explicitly for `--dump-hir` and
  removed the unused sema printer include.
- Confirmed frontend HIR tests already include `hir/hir_printer.hpp`
  explicitly for direct dump assertions.

## Suggested Next

Next packet: Step 7 - update `src/frontend/hir/README.md` to document the
current public facade, private implementation indexes, and inspection/debug
include boundary.

## Watchouts

- `format_hir(...)` still lives in `hir_printer.hpp`; app-facing callers should
  include that public printer header explicitly. The private inspection index
  re-exports that existing printer header for implementation/debug internals.
- Keep `format_summary(...)` in `hir.hpp`; it remains the lightweight public
  summary formatter.
- This slice is include-boundary only; behavior is unchanged.

## Proof

`{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^cpp_hir_'; } > test_after.log 2>&1`

Result: passed for the Step 6 inspection boundary slice. Build completed and
HIR subset proof passed 71/71 `cpp_hir_*` tests. Proof log:
`test_after.log`.
