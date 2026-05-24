Status: Active
Source Idea Path: ideas/open/subsystem-entropy-reduction-refactor-generator.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select And Baseline One Subsystem

# Current Packet

## Just Finished

Step 1 selected `src/backend/mir/aarch64/codegen/` as the first umbrella
entropy-audit subsystem and recorded baseline shape data for Step 2.

Baseline facts:
- Source shape: 77 `.cpp`/`.hpp` files, split as 41 `.cpp` and 36 `.hpp`;
  `scripts/count_src_lines.py | rg 'src/backend/mir/aarch64/codegen|Total:'`
  reports 48,070 lines in this subsystem.
- Largest `.cpp` files by `wc -l`: `alu.cpp` 3,999, `calls_moves.cpp` 3,435,
  `dispatch.cpp` 2,770, `i128_ops.cpp` 2,630, `memory.cpp` 2,547,
  `f128.cpp` 2,070, `machine_printer.cpp` 1,929,
  `instruction.cpp` 1,879, `dispatch_value_materialization.cpp` 1,597,
  `variadic.cpp` 1,471.
- Largest `.hpp` files by `wc -l`: `instruction.hpp` 1,923, `calls.hpp` 401,
  `memory_store_sources.hpp` 158, `dispatch_edge_copies.hpp` 115,
  `i128_ops.hpp` 114, `dispatch_value_materialization.hpp` 103,
  `alu.hpp` 102, `memory.hpp` 101,
  `comparison_branch_fusion.hpp` 95, `dispatch_publication.hpp` 92.
- Broad internal include fan-out is concentrated in `alu.hpp` 28 includers,
  `instruction.hpp` 18, `memory.hpp` 15, `calls.hpp` 15, `dispatch.hpp` 14,
  `machine_printer.hpp` 13, and `dispatch_lookup.hpp` 12. The largest include
  aggregators are `dispatch.cpp` 43 includes, `memory_store_sources.cpp` 31,
  `dispatch_publication.cpp` 27, `calls_dispatch_bridge.cpp` 24,
  `dispatch_value_materialization.cpp` 23, and `dispatch_edge_copies.cpp` 21.
- Route/compatibility search hits: `README.md` explicitly describes legacy
  module surface; `compatibility_projection.*` is legacy compatibility surface;
  `module_compile.cpp` includes compatibility projection; `calls_dispatch_bridge.*`
  names a bridge route and has stack-preserved-value `fallback` logic;
  `alu.cpp` has scalar fallback operand helpers; `memory.cpp` has fallback link
  name handling; `instruction.hpp` keeps compatibility spelling and temporary
  prepared value vocabulary.
- Nearby proof surfaces already present: targeted CTest names in
  `tests/backend/mir/CMakeLists.txt` include `backend_aarch64_*` C++ tests,
  `backend_cli_aarch64_asm_no_machine_nodes_fails`, and optional external
  AArch64 CLI smoke tests; `tests/backend/CMakeLists.txt` also registers
  backend route cases such as
  `backend_codegen_route_aarch64_prepared_call_boundary_scalability` and
  `backend_codegen_route_aarch64_global_function_pointer_table_selected_indirect_call`.
  Step 1 did not run broad validation.

## Suggested Next

Execute Step 2 by turning the AArch64 codegen baseline into one or two bounded
refactor candidates. Start by separating broad ownership surfaces from
low-risk extraction candidates: `dispatch.cpp`, `alu.cpp`, `calls_moves.cpp`,
and `instruction.hpp` are high-entropy hubs, while the route/compatibility
markers around `compatibility_projection.*`, `calls_dispatch_bridge.*`, and
the fallback helpers are good places to define audit questions.

## Watchouts

Keep the next packet planning-only unless the supervisor explicitly delegates
implementation. The selected subsystem is broad enough that Step 2 should avoid
claiming a rewrite target from line count alone; use include fan-out and
route-era terminology to identify a coherent ownership boundary first.

## Proof

Packet proof required: `git diff --check`. No build, test run, or
`test_after.log` is needed for this planning-only packet.
