Status: Active
Source Idea Path: ideas/open/65_aarch64_target_owner_relocation.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Relocate Local-Slot, Frame-Address, And Memory Spelling Helpers

# Current Packet

## Just Finished

Step 3 completed: relocated a coherent AArch64 memory/address spelling helper
group from broad dispatch-publication ownership to the memory owner.

Changes:
- Moved `register_indirect_address`, `fixed_slots_use_frame_pointer`,
  both `frame_slot_address` overloads, `scalar_load_mnemonic`,
  `dispatch_publication_scalar_type_size_bytes`,
  `scalar_load_mnemonic_for_width`, and `scalar_store_mnemonic` declarations
  from `dispatch_publication.hpp` to `memory.hpp`.
- Moved the matching definitions from `dispatch_publication.cpp` to
  `memory.cpp` without behavior changes.
- Kept existing call-site behavior intact by having `dispatch_publication.hpp`
  include the precise memory owner while its own direct declarations shrink.
- Did not touch dispatch sequencing/retry logic, branch-fusion ordering,
  before-return publication ordering, value-publication recursion, tests, or
  idea 67 local-slot behavior.

## Suggested Next

Step 4 should relocate scratch, write-hazard, and publication-spelling helpers
only after choosing a precise target-local owner. Recommended packet: classify
`registers_alias`, publication register recording, block-entry publication
clobber checks, and `value_publication_may_read_register_index`/related hazard
helpers, then move one small group to a publication or value-materialization
owner without changing `emit_value_publication_to_register` recursion.

## Watchouts

`dispatch_publication.hpp` still transitively exposes the moved memory helpers
for existing consumers; later public-surface cleanup can update direct includes
outside this packet. Avoid forwarding-only wrappers, and do not broaden Step 4
into dispatch sequencing, before-return ordering, prepared-memory retry
decisions, current-block join query routing, or shared semantic authority.

The global relocation helper in `dispatch_publication.*` and the private
global-load publication helper in `dispatch_value_materialization.cpp` remain
deferred.

## Proof

Proof passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_records|backend_aarch64_prepared_memory_operand_records|backend_aarch64_memory_operand_contract|backend_aarch64_target_instruction_records|backend_aarch64_return_lowering|backend_codegen_route_aarch64_local_aggregate_address_pointer_copy_publishes_frame_address|backend_codegen_route_aarch64_store_global_stack_publication|backend_codegen_route_aarch64_got_load_global_prepared_memory)$') > test_after.log 2>&1`

`test_after.log` reports 9/9 tests passed. `git diff --check` passed. A
targeted `rg` scan confirms the moved memory/address helper declarations and
definitions live in `memory.*`; remaining `dispatch_publication.cpp` hits are
call sites only.
