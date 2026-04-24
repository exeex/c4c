Status: Active
Source Idea Path: ideas/open/93_hir_agent_index_header_hierarchy.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm Public HIR Surface

# Current Packet

## Just Finished

Step 1 - Confirm Public HIR Surface completed the caller inventory and boundary
decision for the current top-level HIR headers.

- `hir.hpp` is the public facade for HIR pipeline entry points and normal
  summary output: `build_hir(...)` and `format_summary(...)`. Callers are
  `src/frontend/sema/sema.cpp`, `src/apps/c4cll.cpp`, and the HIR facade
  implementation itself. `format_summary(...)` is intentionally public because
  `c4cll --dump-hir-summary` calls it directly.
- `hir_ir.hpp` is the centralized public HIR data-model contract. Its public
  surface is the HIR module graph and backend-facing data model: pipeline
  stage enum, ID/map utilities, source/span/linkage/type/value categories,
  expression and statement node payloads, blocks/functions/globals/program
  summary, struct/template/consteval/module state, inline helpers for
  specialization/type strings, and the DAG summary/model types. It is included
  by codegen, LIR lowering, sema, HIR implementation headers, and HIR tests.
- `hir_printer.hpp` currently exposes only `format_hir(const Module&)`.
  Caller inventory shows it is debug/inspection output for `c4cll --dump-hir`
  and HIR printer-focused tests, not the core facade. It should move behind an
  inspection boundary later, but not in this packet: `src/apps/c4cll.cpp`
  currently reaches `format_hir(...)` transitively through `hir.hpp`, and
  retargeting that app include is outside Step 1 ownership.

## Suggested Next

First code-changing Step 2 sub-slice: introduce
`src/frontend/hir/impl/hir_impl.hpp` as a private compatibility index for the
shared lowering declarations currently in `hir_lowering.hpp`, then retarget
HIR implementation `.cpp` files that include `hir_lowering.hpp` directly. Keep
`hir_lowerer_internal.hpp` movement for the following `impl/lowerer.hpp` slice.

## Watchouts

- Preserve HIR behavior and testcase expectations.
- Do not split `hir_ir.hpp` merely for C++ interface purity.
- Do not create one header per HIR `.cpp`.
- Keep `hir_printer.hpp` movement for the later inspection-boundary step unless
  the supervisor broadens ownership to include `src/apps/c4cll.cpp` and tests.
- `src/frontend/sema/sema.cpp` includes `hir_printer.hpp` but the inspected
  file does not call `format_hir(...)`; that looks removable during the
  inspection slice, not Step 2 private lowering work.
- Keep parser follow-up work from `ideas/open/94_parser_public_facade_pimpl_boundary.md`
  separate.

## Proof

`{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^cpp_hir_'; } > test_after.log 2>&1`

Result: passed with no code changes. HIR subset proof passed 71/71
`cpp_hir_*` tests. Proof log: `test_after.log`.
