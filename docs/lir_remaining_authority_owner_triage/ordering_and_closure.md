# LIR Remaining Authority Owner Triage - Ordering And Closure

Status: Step 3 ordering and closure trace for idea 866
Source: `ideas/open/866_lir_remaining_authority_owner_triage.md`
Evidence:
- `docs/lir_remaining_authority_owner_triage/current_evidence.md`
- `docs/lir_remaining_authority_owner_triage/classification.md`

## Ordering Rule

Run documentation and first-owner producer/schema routes before verifier,
collector, Raw-BIR receiver, and terminal deletion work. 734 stays paused after
accepted receiver commit `750b6b3ba` until a successor accepts one exact typed
handoff. `LirAbsOp` selected-global/i32 remains closed because `0c44e810ad`
already receives it.

## Ordered Successors

1. `ideas/open/841_lir_compact_scalar_abi_leaf_migration.md` - earliest
   executable producer/schema route for compact scalar and ABI-leaf ownership.
2. `ideas/open/842_lir_restricted_first_class_value_unions.md` - producer/schema
   boundary unions after scalar/family prerequisites are accepted.
3. `ideas/open/843_lir_direct_hir_family_construction_array_composition.md` -
   producer construction and recursive array composition after boundary
   alternatives are available.
4. `ideas/open/844_lir_global_extern_initializer_family_facts.md` - global and
   extern type facts after family refs can be produced.
5. `ideas/open/867_lir_memory_va_object_lifetime_authority.md` - fresh
   single-owner producer/schema route for memory/VA/object/lifetime authority.
6. `ideas/open/848_lir_global_policy_identity_evidence.md` - documentation
   evidence for global policy and symbol identity, separate from 844 type facts.
7. `ideas/open/849_lir_intrinsic_binding_evidence.md` - documentation evidence
   for intrinsic and inline-assembly binding while preserving templates and
   constraints as opaque payload.
8. `ideas/open/850_lir_cfg_phi_raw_bindings_evidence.md` - documentation
   evidence for CFG/PHI raw seams before any new receiver route.
9. `ideas/open/845_lir_typed_reference_carriers_collector_migration.md` -
   collector/import-preparation only after exact producer carriers exist.
10. `ideas/open/846_lir_family_overloaded_verifier_dispatch_printer.md` -
    verifier, dispatch, and printer migration after family producers/carriers
    are accepted.
11. `ideas/open/847_lir_universal_model_string_escape_hatch_deletion.md` -
    terminal deletion and handoff to 797 after M1-M15 gates are accepted.
12. `ideas/open/797_lir_to_new_bir_final_coverage_convergence.md` - terminal
    convergence only after all valid families have producer/verifier/receiver
    disposition.

## Reused And Rejected Ideas

Reused current exact-scope successors: 841, 842, 843, 844, 845, 846, 847, 848,
849, and 850. Fresh successor created: 867 for memory/VA/object/lifetime first
ownership. Stale overlaps 795, 796, 813, 821, and 822 are evidence only for
this ordering unless a later exact scope is selected by their own source.

Rejected current successors: direct 734 receiver work, generic residual sweeps,
and `LirAbsOp` selected-global/i32. They either mix first owners, reopen accepted
rows, or lack an accepted typed handoff.

## Deferred Work

734 receiver packets are deferred until a listed first-owner successor accepts
an exact typed handoff. Raw-BIR receiver work must consume one handoff at a
time. Metadata, initializer-text semantics, broad residual instruction sweeps,
and inline-assembly template/constraint parsing remain unowned unless a later
single-owner idea records an exact scope.
