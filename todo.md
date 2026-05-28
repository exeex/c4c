Status: Active
Source Idea Path: ideas/open/65_aarch64_target_owner_relocation.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Relocate Global And Symbol Target Helpers

# Current Packet

## Just Finished

Step 2 completed: relocated AArch64 load-global target lookup and symbol-label
spelling helpers from broad producer ownership to the globals owner.

Changes:
- Moved `find_load_global_target` and `load_global_symbol_label` declarations
  from `dispatch_producers.hpp` to `globals.hpp`.
- Moved their definitions from `dispatch_producers.cpp` to `globals.cpp`
  without behavior changes.
- Removed `globals.cpp`'s producer include; `fp_value_materialization.cpp`
  already includes `globals.hpp` for the relocated helpers and still keeps
  `dispatch_producers.hpp` for `producer_instruction_index`.
- Confirmed `dispatch_producers.*` no longer owns or exposes the global target
  helpers. Remaining call sites are in `globals.cpp` and
  `fp_value_materialization.cpp`.

## Suggested Next

Step 3 should relocate the next memory/address spelling group from
`dispatch_publication.*` into precise target owners. Recommended first cut:
move scalar load/store mnemonic helpers and frame/register-indirect address
formatting toward `memory.*` or another existing address/memory owner, while
leaving branch-fusion sequencing, before-return publication ordering, and
value-publication recursion untouched.

## Watchouts

The private global-load publication helper in
`dispatch_value_materialization.cpp` remains deferred; moving it would start
pulling on broad value-publication ownership. Keep Step 3 focused on
memory/address spelling and avoid forwarding-only renames.

Preserve global load spelling, GOT-required/direct labels, frame-slot address
spelling, fixed-formal stores, scalar load/store mnemonics, local-slot/global
materialization, scratch hazards, prepared-memory retry routing, and
address-materialization retry routing. Do not move semantic producer discovery
or current-block join prepared-query routing.

## Proof

Proof passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_prepared_memory_operand_records|backend_aarch64_target_instruction_records|backend_codegen_route_aarch64_got_load_global_prepared_memory|backend_codegen_route_aarch64_global_function_pointer_table_selected_indirect_call|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication)$') > test_after.log 2>&1`

`test_after.log` reports 6/6 tests passed. `git diff --check` passed. A
targeted `rg` scan confirms the relocated helper names are absent from
`dispatch_producers.*` and present only in `globals.*` plus direct consumers.
