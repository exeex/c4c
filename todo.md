Status: Active
Source Idea Path: ideas/open/68_aarch64_large_owner_residue_audit.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Draft Follow-Up Cleanup Ideas

# Current Packet

## Just Finished

Step 4 - Draft Follow-Up Cleanup Ideas completed audit-only follow-up idea
creation from the Step 3 classification table. The drafts are grouped by one
owner family or one shared-authority family; no monolithic mixed cleanup idea
was created.

Created follow-up ideas:

| Idea | Route type | Scope |
| --- | --- | --- |
| `ideas/open/69_aarch64_call_publication_prepared_authority_cleanup.md` | shared-authority migration | `calls.cpp`, `dispatch_publication.cpp`, prepared call/move/publication facts |
| `ideas/open/70_aarch64_memory_prepared_address_authority_cleanup.md` | shared-authority migration | `memory.cpp`, memory-backed f128/variadic consumers, prepared address/value-home/storage/stack-source facts |
| `ideas/open/71_aarch64_scalar_control_flow_prepared_authority_cleanup.md` | shared-authority migration | `alu.cpp`, `cast_ops.cpp`, `comparison.cpp`, BIR scalar facts and prepared scalar/control-flow facts |
| `ideas/open/72_aarch64_special_carrier_prepared_policy_cleanup.md` | shared-authority migration | `i128_ops.cpp`, `f128.cpp`, prepared i128/f128 carrier and runtime-helper policy |
| `ideas/open/73_aarch64_variadic_prepared_entry_plan_cleanup.md` | shared-authority migration | `variadic.cpp`, prepared variadic entry-plan and helper operand-home facts |
| `ideas/open/74_aarch64_local_scalar_register_helper_fold_back.md` | mechanical target cleanup | AArch64-local scalar type, register-view, and compare predicate helper duplication |
| `ideas/open/75_shared_aggregate_transport_plan_probe.md` | evidence probe / missing shared authority | aggregate copy chunk/lane transport planning across `instruction.cpp` and `memory.cpp` |
| `ideas/open/76_aarch64_publication_ordering_evidence_probe.md` | evidence probe | publication materialization ordering across dispatch, calls, and memory sources |
| `ideas/open/77_aarch64_machine_status_printer_validation_probe.md` | evidence probe | machine status derivation and printer validation overlap |

Each follow-up names owned files, explicit non-goals, proof expectations, and
concrete reviewer reject signals for testcase-shaped shortcuts, expectation
downgrades, classification-only changes, broad mixed rewrites, and renamed
old failure modes.

## Suggested Next

Proceed to Step 5: close the audit route through lifecycle ownership. Summarize
the Step 3 classification and Step 4 follow-up list, explain why
dispatch-family cleanup is not being reopened as the primary next route,
confirm that no implementation files changed as part of the audit, and close
the source idea only if the lifecycle owner accepts the audit as complete.

## Watchouts

- Audit-only route: no implementation files were edited for this packet.
- `src/backend/mir/arm/` is absent in this checkout, so the classification is
  based on Step 2's RISC-V/x86 reference notes plus shared prepared authority.
- Machine records and printer spelling remain `target-emission` unless a
  narrow probe proves duplicated semantic validation.
- Aggregate transport is the only clear `missing-shared-authority` entry from
  the Step 2 notes; it should not be converted into implementation work until a
  follow-up idea names the exact prepared aggregate-transport query shape.
- The follow-up idea numbering starts at 69 because `ideas/open/68_...` is the
  active audit source idea and the highest existing idea number in this
  checkout.

## Proof

No build/test required for this audit-only Step 4 follow-up draft creation; no
`test_after.log` was produced for the delegated proof.
Proof command: `git diff --check -- todo.md ideas/open/69_aarch64_call_publication_prepared_authority_cleanup.md ideas/open/70_aarch64_memory_prepared_address_authority_cleanup.md ideas/open/71_aarch64_scalar_control_flow_prepared_authority_cleanup.md ideas/open/72_aarch64_special_carrier_prepared_policy_cleanup.md ideas/open/73_aarch64_variadic_prepared_entry_plan_cleanup.md ideas/open/74_aarch64_local_scalar_register_helper_fold_back.md ideas/open/75_shared_aggregate_transport_plan_probe.md ideas/open/76_aarch64_publication_ordering_evidence_probe.md ideas/open/77_aarch64_machine_status_printer_validation_probe.md`
Result: passed after repair rerun.
