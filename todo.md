Status: Active
Source Idea Path: ideas/open/131_aarch64_dispatch_edge_copy_helper_surface_privatization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reconfirm Edge-Copy Public Surface
你該做code review了

# Current Packet

## Just Finished

Completed plan Step 1: Reconfirm Edge-Copy Public Surface.

Declaration/call-site table for
`src/backend/mir/aarch64/codegen/dispatch_edge_copies.hpp`:

| Declaration | Direct callers / references | Candidate action | Rationale |
| --- | --- | --- | --- |
| `struct EdgeProducerContext` | Constructed and passed only inside `dispatch_edge_copies.cpp` (`prepared_edge_publication_producer_context`, `prepared_edge_source_producer_context`, `prepared_edge_named_source_producer_context`, edge-load helpers). No direct external references found outside the header. | file-local | This is implementation plumbing for prepared edge-publication producer lookup and is only needed by edge-copy implementation helpers. |
| `edge_value_publication_may_read_register_index(...)` | AST caller query in `dispatch_edge_copies.cpp` reports only `emit_edge_value_publication_to_register_impl`, self-recursion, and `should_emit_block_entry_edge_copy_move`; focused `rg` finds no direct external call. | file-local | The hazard query is consumed by edge-copy implementation and the block-entry edge-copy filter in the same translation unit. |
| `emit_edge_load_local_to_register(...)` | AST caller query in `dispatch_edge_copies.cpp` reports no callers; focused `rg` finds only declaration, definition, and internal impl references. | file-local | The public wrapper has no current direct caller. If retained after Step 2, it should be private implementation surface. |
| `emit_edge_value_publication_to_register(...)` | AST caller query in `dispatch_edge_copies.cpp` reports only anonymous-namespace `lower_predecessor_join_source_publication`; focused `rg` finds no direct external call. | file-local | Publication emission is invoked from edge-copy lowering internals, not from cross-file dispatch code. |
| `should_emit_block_entry_edge_copy_move(...)` | AST caller query in `dispatch.cpp` reports `dispatch_prepared_block`; raw call site is `dispatch.cpp:504`. No direct callers inside `dispatch_edge_copies.cpp`. | dispatch-orchestration-only | This is a real current cross-file hook only because block dispatch asks whether to keep prepared block-entry moves. Candidate for Step 3 fold-back into `dispatch.cpp` only if dependencies remain cleaner than the current split; otherwise keep public with this caller-based rationale. |
| `lower_predecessor_select_parallel_copy_sources(...)` | AST caller query in `dispatch.cpp` reports `dispatch_prepared_block`; raw call site is `dispatch.cpp:985`. No direct callers inside `dispatch_edge_copies.cpp`. | dispatch-orchestration-only | This is a real current cross-file hook for branch dispatch predecessor select parallel-copy emission. Candidate for Step 3 fold-back into `dispatch.cpp` only if moving the implementation does not obscure edge-copy/publication ownership. |

Additional include-only observation: `dispatch_edge_copies.hpp` is included by
`alu.cpp`, `calls.cpp`, `dispatch_producers.cpp`, `dispatch_value_materialization.cpp`,
and `memory.cpp`, but focused declaration-name search found no direct use of
the public edge-copy declarations in those files.

## Suggested Next

Execute Step 2: Privatize File-Local Helpers. Start with
`EdgeProducerContext`, `edge_value_publication_may_read_register_index`,
`emit_edge_load_local_to_register`, and
`emit_edge_value_publication_to_register`, preserving behavior and preparing a
matching build proof.

## Watchouts

- Preserve edge-copy instruction order, predecessor select parallel-copy
  behavior, and block-entry publication behavior.
- Do not replace prepared edge-copy or publication facts with local
  rediscovery.
- Do not move AArch64 register hazard policy or final instruction spelling into
  shared code.
- Use matching `test_before.log` and `test_after.log` if public header or
  translation-unit shape changes.
- The two dispatch-only hooks are real cross-file calls today because
  `dispatch_prepared_block` uses them. Do not remove them from the header until
  either their implementations move to `dispatch.cpp` or replacement ownership
  is proven clean.
- Several files include `dispatch_edge_copies.hpp` without using the audited
  declarations directly; Step 4 should clean stale includes after the public
  surface contraction is known.

## Proof

Analysis-only packet; no implementation files changed and no build/test run was
required.

Proof commands/results:

- `sed -n '1,220p' src/backend/mir/aarch64/codegen/dispatch_edge_copies.hpp`
  listed one public type declaration and five public function declarations.
- `rg -n '\bEdgeProducerContext\b|\bedge_value_publication_may_read_register_index\b|\bemit_edge_load_local_to_register\b|\bemit_edge_value_publication_to_register\b|\bshould_emit_block_entry_edge_copy_move\b|\blower_predecessor_select_parallel_copy_sources\b' src/backend/mir/aarch64/codegen/dispatch.cpp src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp src/backend/mir/aarch64/codegen/dispatch_edge_copies.hpp`
  confirmed declaration references, internal implementation references, and
  only two external call sites in `dispatch.cpp`.
- `rg -n '#include "dispatch_edge_copies.hpp"|dispatch_edge_copies.hpp' src/backend/mir/aarch64/codegen src/backend -g '*.cpp' -g '*.hpp' -g 'CMakeLists.txt'`
  identified include-only consumers to revisit after contraction.
- `c4c-clang-tool-ccdb function-callers ... dispatch_edge_copies.cpp edge_value_publication_may_read_register_index ...`
  reported only same-translation-unit callers/self-recursion.
- `c4c-clang-tool-ccdb function-callers ... dispatch_edge_copies.cpp emit_edge_load_local_to_register ...`
  reported `target not called in this translation unit`.
- `c4c-clang-tool-ccdb function-callers ... dispatch_edge_copies.cpp emit_edge_value_publication_to_register ...`
  reported only `lower_predecessor_join_source_publication`.
- `c4c-clang-tool-ccdb function-callers ... dispatch_edge_copies.cpp should_emit_block_entry_edge_copy_move ...`
  and `... lower_predecessor_select_parallel_copy_sources ...` reported no
  callers inside `dispatch_edge_copies.cpp`.
- `c4c-clang-tool-ccdb function-callers ... dispatch.cpp should_emit_block_entry_edge_copy_move ...`
  and `... dispatch.cpp lower_predecessor_select_parallel_copy_sources ...`
  each reported `dispatch_prepared_block`.

`test_after.log` was not updated because this packet explicitly listed root
proof logs under Do Not Touch; proof output is recorded here instead.
