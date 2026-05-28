Status: Active
Source Idea Path: ideas/open/65_aarch64_target_owner_relocation.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Relocate Scratch, Write-Hazard, And Publication Spelling Helpers

# Current Packet

## Just Finished

Step 4 completed: relocated a small AArch64 register aliasing/publication
hazard helper group from broad dispatch-publication ownership to the ALU owner.

Changes:
- Moved `registers_alias` and
  `register_operands_share_physical_register` declarations from
  `dispatch_publication.hpp` to `alu.hpp`.
- Moved their definitions from `dispatch_publication.cpp` to `alu.cpp`; the
  physical-register helper now delegates to `registers_alias`.
- Confirmed `dispatch_publication.*` no longer declares or defines this helper
  group; remaining hits there are call sites only.
- Did not create forwarding wrappers, add a new source file, touch dispatch
  sequencing/retry logic, or change `dispatch_value_materialization.cpp`
  recursion.

## Suggested Next

Step 5 should retire or justify remaining dispatch-family adapter surface.
Recommended packet: scan `dispatch_publication.hpp`,
`dispatch_value_materialization.hpp`, and `dispatch_producers.hpp` for helpers
that are now only transitively exposed after Steps 2-4, update direct includes
where ownership is clear, and remove obsolete broad declarations without
renaming wrappers or changing behavior.

## Watchouts

No `publication.*` owner existed in this checkout, so the packet used the
existing ALU owner for the target-local register aliasing group rather than
adding a new build-system source. Step 5 should avoid forwarding-only wrappers
and should not broaden into dispatch sequencing, before-return ordering,
prepared-memory retry decisions, current-block join query routing, or shared
semantic authority.

The global relocation helper in `dispatch_publication.*` and the private
global-load publication helper in `dispatch_value_materialization.cpp` remain
deferred.

## Proof

Proof passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_scalar_alu_records|backend_aarch64_prepared_scalar_alu_records|backend_aarch64_scalar_cast_records|backend_aarch64_prepared_scalar_cast_records|backend_aarch64_branch_compare_records|backend_aarch64_prepared_branch_records|backend_aarch64_return_lowering|backend_store_source_publication_plan|backend_codegen_route_aarch64_store_global_stack_publication|backend_codegen_route_aarch64_scalar_fp_literal_add_publishes_fpr_result)$') > test_after.log 2>&1`

`test_after.log` reports 11/11 tests passed. `git diff --check` passed. A
targeted `rg` scan confirms the relocated helper declarations and definitions
live in `alu.*`; remaining `dispatch_publication.cpp` hits are call sites only.
