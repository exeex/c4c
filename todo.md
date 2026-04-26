Status: Active
Source Idea Path: ideas/open/116_bir_layout_dual_path_coverage_and_dump_guard.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Cover Aggregate Storage And Initializer Paths

# Current Packet

## Just Finished

Completed `plan.md` Step 3 global aggregate storage and initializer coverage.

Registered the existing global aggregate fixtures as semantic BIR route tests:
defined global struct stores, global struct array read/store, and nested global
struct array read/store. The new guards assert concrete global offsets for
field and array addressing (`@v+4`, `@pairs+8`, `@pairs+12`, `@s+4`, and
`@s+8`) while the delegated subset continues to prove the existing local
aggregate memcpy/memset initializer paths. Backend behavior was not changed;
legacy `type_decls` remain fallback/parity evidence.

## Suggested Next

Take one small Step 3 packet for local aggregate slot/address coverage not
already covered by memcpy/memset, then decide whether Step 3 is ready to hand
to plan review or move to Step 4.

## Watchouts

- Keep legacy `type_decls` available as fallback and parity evidence.
- Do not remove legacy parsing helpers during this runbook.
- Treat `--dump-bir` tests as guards for lowered BIR facts, not as a BIR
  printer render-context migration.
- Do not downgrade expectations or add named-case shortcuts.
- The new global tests are CMake registrations for existing fixtures; they do
  not add backend behavior or change case source semantics.
- Local memcpy/memset coverage proves initializer-derived local storage paths,
  but a direct non-intrinsic local slot/addressing guard may still be useful
  before Step 3 is considered exhausted.

## Proof

Proof command run:
`{ cmake --build build-backend && ctest --test-dir build-backend -j --output-on-failure -R '^(frontend_lir_|backend_codegen_route_x86_64_(defined_global_struct_store|global_struct_array_(read|store)|nested_global_struct_array_(read|store)|builtin_memcpy_(local_pair|nested_pair_field|local_i32_array|nested_i32_array_field)|builtin_memset_(local_pair|local_i32_array|nested_i32_array_field)|builtin_memset_nonzero_(local_pair|local_i32_array|nested_i32_array_field))_observe_semantic_bir$)'; } > test_after.log 2>&1`

Result: passed. Build completed and 19/19 selected tests passed.
Proof log: `test_after.log`.
