Status: Active
Source Idea Path: ideas/open/116_bir_layout_dual_path_coverage_and_dump_guard.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Cover Aggregate Storage And Initializer Paths

# Current Packet

## Just Finished

Completed `plan.md` Step 3 local aggregate slot/address coverage for direct
struct member offsets.

Added `local_aggregate_member_offsets.c` and registered it as a semantic BIR
backend route test. The guard observes concrete local stores and loads at
`%lv.p.0` and `%lv.p.4` for initializer stores, a direct member overwrite, and
direct member reads. Backend behavior was not changed.

## Suggested Next

Ask the supervisor to decide whether Step 3 is now ready for plan review or
whether one final adjacent aggregate-storage guard is still needed before
moving to Step 4.

## Watchouts

- Keep legacy `type_decls` available as fallback and parity evidence.
- Do not remove legacy parsing helpers during this runbook.
- Treat `--dump-bir` tests as guards for lowered BIR facts, not as a BIR
  printer render-context migration.
- Do not downgrade expectations or add named-case shortcuts.
- This packet added only a test fixture and CMake registration; any future
  failing local aggregate guard should be treated as a general backend issue,
  not patched through a named-case shortcut.

## Proof

Proof command run:
`{ cmake --build build-backend && ctest --test-dir build-backend -j --output-on-failure -R '^(frontend_lir_|backend_codegen_route_x86_64_(local_aggregate_member_offsets|local_dynamic_member_array|local_dynamic_member_array_store|local_direct_dynamic_member_array_(load|store)|local_direct_dynamic_struct_array_call|builtin_memcpy_(local_pair|nested_pair_field|local_i32_array|nested_i32_array_field)|builtin_memset_(local_pair|local_i32_array|nested_i32_array_field)|builtin_memset_nonzero_(local_pair|local_i32_array|nested_i32_array_field))_observe_semantic_bir$)'; } > test_after.log 2>&1`

Result: passed. Build completed and 20/20 selected tests passed.
Proof log: `test_after.log`.
