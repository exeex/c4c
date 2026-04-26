Status: Active
Source Idea Path: ideas/open/116_bir_layout_dual_path_coverage_and_dump_guard.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Make Safe Structured Layout Lookups First-Class

# Current Packet

## Just Finished

Completed `plan.md` Step 2 packed structured-layout metadata repair.

HIR `pack_align` now propagates into LIR structured declarations, and the LIR
shadow declaration prints packed LLVM aggregate syntax for packed records. The
backend legacy layout parser now understands packed `<{ ... }>` shadows so the
existing structured-vs-legacy parity gate can accept the first-class structured
layout instead of falling back. The focused
`backend_codegen_route_x86_64_packed_local_member_offsets_observe_semantic_bir`
guard proves a packed local member array lowers at byte offsets `1`, `5`, and
`9` rather than natural offsets.

## Suggested Next

Ask the supervisor whether Step 2 now has enough safe structured layout lookup
coverage to move to the dump guard work, or take one final small Step 2 packet
for packed pointer/member access if more packed coverage is desired.

## Watchouts

- Keep legacy `type_decls` available as fallback and parity evidence.
- Do not remove legacy parsing helpers during this runbook.
- Treat `--dump-bir` tests as guards for lowered BIR facts, not as a BIR
  printer render-context migration.
- Do not downgrade expectations or add named-case shortcuts.
- The repaired route depends on keeping storage provenance distinct from the
  leaf aggregate type: the address byte offset is still relative to the runtime
  pointer base, while array extent lookup may need the parent storage layout.
- The backend `types.cpp` change is parser support for packed legacy shadows,
  not a relaxation of `lookup_backend_aggregate_type_layout` parity policy.

## Proof

Proof command run:
`{ cmake --build build-backend && ctest --test-dir build-backend -j --output-on-failure -R '^(frontend_lir_|positive_sema_ok_pragma_pack_c|backend_codegen_route_x86_64_(nested_member_pointer_array|nested_pointer_param_dynamic_member_array_load|local_dynamic_member_array|local_dynamic_member_array_store|local_direct_dynamic_member_array_store|local_direct_dynamic_member_array_load|local_direct_dynamic_struct_array_call|packed_local_member_offsets)_observe_semantic_bir$)'; } > test_after.log 2>&1`

Result: passed. Build completed and 13/13 selected tests passed.
Proof log: `test_after.log`.
