Status: Active
Source Idea Path: ideas/open/102_hir_struct_method_member_identity_dual_lookup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Structured Mirrors for Struct Definitions

# Current Packet

## Just Finished

Completed the first Step 3 slice by dual-writing structured record-owner mirrors for struct definitions without changing existing rendered-name authority.

Added:
- `Module::struct_def_owner_index`, `struct_def_owner_order`, and `struct_def_owner_parity_mismatches` beside `struct_defs` / `struct_def_order`.
- `Module` helpers for structured struct-definition lookup and rendered-vs-structured parity recording.
- Lowerer-side `struct_def_nodes_by_owner_` plus registration helpers for AST struct-definition node mirrors.
- Dual-writes from ordinary `lower_struct_def`, initial struct-node collection, and template struct instantiation when declaration/template key metadata is complete.

Existing rendered-name maps, `struct_def_order`, lookup behavior, HIR dumps, codegen spelling, tests, and expectation files remain authoritative and unchanged.

## Suggested Next

Continue Step 3 by adding structured mirrors for `template_struct_defs_` and `template_struct_specializations_`, then start focused dual-read/parity probes that compare structured and rendered lookups without redirecting existing consumers.

## Watchouts

Keep `TypeSpec::tag`, `HirStructDef::tag`, `Module::struct_defs`, and `struct_def_order` as the rendered output-spelling bridge. This slice records parity state and structured lookup helpers, but no existing reader has been redirected. Explicit template-specialization AST-node mirrors still use declaration keys unless a later slice supplies full specialization identity at registration time.

## Proof

Ran exactly `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -L hir) > test_after.log 2>&1`.

Result: passed. `test_after.log` reports 73/73 HIR-labeled tests passing.
