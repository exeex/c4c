# LIR Remaining Authority Owner Triage - First Owner Classification

Status: Step 2 classification for idea 866
Source: `ideas/open/866_lir_remaining_authority_owner_triage.md`
Evidence: `docs/lir_remaining_authority_owner_triage/current_evidence.md`

## Classification Rule

Raw-BIR receipt is never the first owner for a family whose LIR fact is still
raw, textual, schema-incomplete, or unproved. A family may return to
`ideas/open/734_lir_to_new_bir_container_completeness.md` only after the first
owner has published an exact typed LIR handoff or an evidence document proving
the existing handoff boundary. Producer/schema/verifier work stays separate
from Raw-BIR receiver work.

## Dependency Order

1. Documentation or research routes that map unknown seams before executable
   ownership is selected.
2. LIR producer/schema routes that create native facts or family-specific
   carriers.
3. LIR verifier routes that enforce already-published native facts without
   reconstructing them from text.
4. Raw-BIR receiver routes under 734 for one accepted typed handoff at a time.
5. Importer or terminal convergence work only after producer and receiver
   dispositions are complete.

## Family Classifications

| Family | First owner | Current successor | Receiver separation | Blocker / disposition |
| --- | --- | --- | --- | --- |
| CFG/PHI residuals | Documentation first, then LIR producer/schema or verifier for exact seams | `ideas/open/850_lir_cfg_phi_raw_bindings_evidence.md` | 734 receives only after 850 or a later exact handoff proves a typed seam | Not currently executable as a receiver route. Accepted CFG/PHI rows must not be reopened, and remaining raw PHI/terminator seams need a per-form evidence map before implementation scope. |
| Memory/VA and object/lifetime authority | LIR producer/schema for new object/lifetime/VA facts; documentation when the exact seam is unknown | No current exact successor from the inspected set | 734 is downstream only after a typed object/lifetime or VA handoff exists | Not currently executable. Local-object and VLA rows are already accepted or stale; residual memory/VA needs a fresh single-owner idea instead of a receiver packet. |
| Aggregate/vector value and type authority | LIR producer/schema before verifier or receiver | `ideas/open/842_lir_restricted_first_class_value_unions.md`, `ideas/open/843_lir_direct_hir_family_construction_array_composition.md` | Raw-BIR receipt waits for accepted boundary unions or direct HIR family construction | Current successors exist for first-owner producer/schema work. They must not be collapsed into 734, 796, or generic receiver coverage. |
| Module/type/global/metadata authority | LIR producer/schema for type/global facts; documentation/research for policy, identity, and non-type globals | `ideas/open/841_lir_compact_scalar_abi_leaf_migration.md`, `ideas/open/844_lir_global_extern_initializer_family_facts.md`, `ideas/open/845_lir_typed_reference_carriers_collector_migration.md`, `ideas/open/848_lir_global_policy_identity_evidence.md` | 734 receives only after family refs, carriers, or policy evidence establish exact typed fields | Executable only through the listed first-owner routes. Metadata and initializer-text semantics remain non-executable until a narrower evidence route selects ownership. |
| Residual instruction/terminator authority | LIR producer/schema for selected instruction families; LIR verifier/dispatch after schemas exist; documentation for unproved inventories | `ideas/open/796_lir_instruction_terminator_residual_authority_handoff.md`, `ideas/open/846_lir_family_overloaded_verifier_dispatch_printer.md`, `ideas/open/849_lir_intrinsic_binding_evidence.md`, `ideas/open/850_lir_cfg_phi_raw_bindings_evidence.md` | 734 is not first owner for residual instructions; it receives one accepted typed handoff only after producer/verifier work | Partly executable through selected single-family producer routes under 796. Generic residual sweeps are rejected as currently non-successors. |
| Inline-assembly | Documentation first for binding inventory; LIR producer/schema or verifier only for exact native binding gaps | `ideas/open/849_lir_intrinsic_binding_evidence.md` | 734 can receive only a later exact binding handoff, not templates or constraints | Not currently executable as implementation. Templates and constraints stay opaque; parsing them for value, type, ABI, or dispatch facts is rejected. |
| Generic residual sweeps | Broader policy/documentation, not implementation | No current executable successor | No receiver route | Rejected as current successors. They mix unrelated first owners and would risk reopening accepted rows or hiding missing producer/schema authority behind receiver work. |

## Stale Open Idea Reconciliation

`ideas/open/795_lir_body_parameter_authority_handoff.md` is stale for current
866 successor selection. Its selected body-parameter/index handoff is recorded
complete and returns to the older 810 chain, not post-Step-7.51 734. Body
parameters and accepted direct-call parameter rows must not be reopened here.

`ideas/open/796_lir_instruction_terminator_residual_authority_handoff.md`
remains a possible current successor only for a newly selected bounded
residual instruction/terminator producer family. Its accepted cast-result and
pointer-subtraction records are evidence, not blanket ownership of every
residual instruction, terminator, or inline-assembly seam.

`ideas/open/797_lir_to_new_bir_final_coverage_convergence.md` is not a current
implementation successor. It is terminal dispatcher/proof convergence after
every valid current-LIR family has a disposition and any needed 734 receipts
are accepted.

`ideas/open/813_lir_string_semantic_authority_completion_umbrella.md` is an
older string-authority routing umbrella. It remains useful evidence for
generated owners 841-850, but it is not itself the current 734 return route for
866.

`ideas/open/821_frontend_lir_manual_switch_modelled_result_authority.md` and
`ideas/open/822_lowering_produced_lir_switch_selector_authority.md` are stale
overlaps for switch-selector authority. They belong to the 820/821/822 return
chain and pending local-slice history, not the current 734 post-7.51 successor
set.

`ideas/open/841_lir_compact_scalar_abi_leaf_migration.md` is a current
producer/schema successor for compact scalar and ABI-leaf first ownership. It
must complete before dependent family migration or deletion gates rely on
scalar refs.

`ideas/open/842_lir_restricted_first_class_value_unions.md` is a current
producer/schema successor for bounded value carriers at call, PHI, select, and
return boundaries. Raw-BIR receiver work follows only after those alternatives
are accepted.

`ideas/open/843_lir_direct_hir_family_construction_array_composition.md` is a
current producer successor for HIR-to-LIR family construction and recursive
array composition. It is not a collector, receiver, or verifier deletion route.

`ideas/open/844_lir_global_extern_initializer_family_facts.md` is a current
producer/schema successor for global and extern type facts. It explicitly
does not own initializer-text semantics or non-type global policy.

`ideas/open/845_lir_typed_reference_carriers_collector_migration.md` is a
current collector/import-preparation successor after exact producers publish
semantic carriers. It is not first owner for missing producer facts.

`ideas/open/846_lir_family_overloaded_verifier_dispatch_printer.md` is a
current verifier/dispatch/printer successor after family producers and carriers
exist. It must not create producer state or parse display text as semantics.

`ideas/open/847_lir_universal_model_string_escape_hatch_deletion.md` is a
terminal deletion and disposition handoff. It is not currently executable for
866 until M1-M15 replacement gates are accepted.

`ideas/open/848_lir_global_policy_identity_evidence.md` is a current
documentation/research successor for global policy and symbol-identity
evidence. Any later 734 receipt depends on its field-by-field producer,
verifier, and receiver trace.

`ideas/open/849_lir_intrinsic_binding_evidence.md` is a current
documentation/research successor for intrinsic and inline-assembly binding
inventory. It preserves inline-assembly templates and constraints as opaque
payload.

`ideas/open/850_lir_cfg_phi_raw_bindings_evidence.md` is a current
documentation/research successor for CFG/PHI and terminator raw-binding
evidence. It must distinguish accepted 734 bounded receipts from unproved raw
seams before any new receiver route is selected.

## Raw-BIR Receiver Boundary

734 should be re-entered only for a single accepted typed LIR handoff. The
current executable first owners are producer/schema or documentation routes,
with verifier and collector routes ordered after native facts exist. No family
above authorizes a direct Raw-BIR receiver packet from idea 866 Step 2.
