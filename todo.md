Status: Active
Source Idea Path: ideas/open/116_bir_layout_dual_path_coverage_and_dump_guard.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Make Safe Structured Layout Lookups First-Class

# Current Packet

## Just Finished

Completed `plan.md` Step 2 pointer-parameter nested dynamic member-array
capability repair.

Runtime pointer GEP provenance now keeps the parent aggregate storage type when
a constant GEP reaches an aggregate subobject at a nonzero byte offset, and
seeds missing storage type from the current GEP element type for loaded runtime
pointers. This lets `p->inner->xs[i]` lower through semantic BIR dynamic
selection over all three elements at `addr %t1+4`, `addr %t1+8`, and
`addr %t1+12` instead of truncating the accessible extent after the first
nonzero field offset.

## Suggested Next

Continue `plan.md` Step 2 with another small safe structured layout lookup
family, or ask the plan owner whether Step 2 now has enough dual-path coverage
to move to the dump guard work.

## Watchouts

- Keep legacy `type_decls` available as fallback and parity evidence.
- Do not remove legacy parsing helpers during this runbook.
- Treat `--dump-bir` tests as guards for lowered BIR facts, not as a BIR
  printer render-context migration.
- Do not downgrade expectations or add named-case shortcuts.
- The repaired route depends on keeping storage provenance distinct from the
  leaf aggregate type: the address byte offset is still relative to the runtime
  pointer base, while array extent lookup may need the parent storage layout.

## Proof

Proof command run:
`{ cmake --build build-backend && ctest --test-dir build-backend -j --output-on-failure -R '^backend_codegen_route_x86_64_(nested_member_pointer_array|local_dynamic_member_array|local_dynamic_member_array_store|local_direct_dynamic_member_array_store|local_direct_dynamic_member_array_load|local_direct_dynamic_struct_array_call|nested_pointer_param_dynamic_member_array_load)_observe_semantic_bir$'; } > test_after.log 2>&1`

Result: passed. Build completed and 7/7 selected tests passed.
Proof log: `test_after.log`.
