Status: Active
Source Idea Path: ideas/open/55_aarch64_scalar_cast_alu_publication_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Trace The First Bad Publication

# Current Packet

## Just Finished

Step 1 - Trace The First Bad Publication: recorded the current scalar cast/ALU
publication boundary after prerequisite closures. The focused boundary is
currently green across the ALU unpublished-load-local probes, aggregate byte-copy
backend routes, and the four AArch64 C tests including `00204`.

The stale cast/ALU publication first bad fact is not currently reproducible in
the delegated proof subset. No owned cast, ALU, or prepared-preservation first
bad fact remains classified for implementation in this packet.

## Suggested Next

Supervisor/plan-owner closure review is recommended over implementation. If the
source idea still needs work, the next packet should first identify new failing
evidence outside this green boundary before touching cast, ALU, prepared lookup,
or preservation code.

## Watchouts

- Do not turn the `00204` route into named-case matching.
- Do not claim unrelated dirty `memory.cpp` or `dispatch_edge_copies.cpp`
  changes as progress under this idea unless the supervisor explicitly routes a
  combined slice.
- Keep routine execution progress in this file rather than rewriting
  `plan.md`.
- Current proof only classifies the delegated eight-test boundary; it does not
  prove broader backend closure.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy|backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy|backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'`

Result: build succeeded; focused subset is 8/8 passing. Proof log:
`test_after.log`.
