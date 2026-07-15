# Post-7.30 Bounded Successor Queue

Status: final 793 queue; derived from the post-792/post-7.30 baseline and
first-owner matrix.

## Queue rule

This is a dependency-ordered queue, not an authorization to receive every
family in 734.  A producer/type-model idea must publish an accepted, checked,
single-row structured handoff before 734 may be repaired for that row.  Neither
runtime text nor a `monostate`/unclassified operand establishes receiver
authority.

| Order | First owner and source | Bounded output / dependency | 734 relationship |
| --- | --- | --- | --- |
| 1 | Producer/schema/verifier — `ideas/open/794_lir_next_local_vla_authority_handoff.md` | Select and publish exactly one remaining local/VLA row only when native authority proves it. | First future return: after 794 accepts a handoff naming exactly one row, reactivate 734 and repair it for only that handoff's typed Raw-BIR receipt. |
| 2 | Producer/schema/verifier — existing `ideas/open/753_lir_memory_va_pointer_authority_convergence.md` | Structured memory/va pointer, object, and lifetime authority after closed 752. | A later one-row receiver handoff; selected memcpy does not authorize residual rows. |
| 3 | Type model — existing `ideas/open/763_lir_composite_type_ref_model.md` | Composite `LirTypeRef` carrier for shared non-builtin type facts. | Prerequisite only; it is not a receiver row. |
| 4 | Producer/schema/verifier — existing `ideas/open/754_lir_aggregate_vector_value_identity_convergence.md` | Aggregate/vector result/use facts; consume 763 where a composite type is required. | A later one-row receiver handoff; no aggregate/vector row is pre-authorized. |
| 5 | Producer/schema/verifier — `ideas/open/795_lir_body_parameter_authority_handoff.md` | Function-body parameter identity and ABI-expanded-form classification. | A later one-row receiver handoff; closed 742's declaration facts are insufficient. |
| 6 | Type model — existing `ideas/open/761_lir_call_signature_type_mirror_convergence.md` | Call/signature typed mirrors after closed 759/760 and as needed by later residuals. | Prerequisite only; opaque asm text remains text. |
| 7 | Type model/module convergence — existing `ideas/open/762_lir_module_declaration_type_shadow_convergence.md` | Module declaration/global/type shadows after 761; consume 763 when required. | A later module/global one-row handoff only when a producer names it. |
| 8 | Producer/schema/verifier — `ideas/open/796_lir_instruction_terminator_residual_authority_handoff.md` | One residual instruction, terminator, or inline-asm value/type binding family per accepted handoff. | Each accepted handoff creates a separate later 734 receiver repair; never parse opaque templates/constraints. |
| 9 | Dispatcher/proof then documentation — `ideas/open/797_lir_to_new_bir_final_coverage_convergence.md` | No-omission matrix, dispatcher and verifier completeness, whole-module proof, and documentation convergence after all prior handoffs/receipts. | Terminal convergence; cannot authorize an earlier receiver row. |

## Exact 734 future one-row return path

The immediate next parent route is only 794.  After 794 closes with accepted
producer/verifier proof and a handoff that identifies **one** local/VLA variant,
its native value/object/owner/type/liveness fields, rejected forms, and focused
proof, reactivate `ideas/open/734_lir_to_new_bir_container_completeness.md`.
Repair its runbook for one Raw-BIR receiver packet that consumes only those
published fields, adds the minimum destination/importer/reachable-verifier
work, and proves transactional positive and malformed-authority rejection.
Preserve accepted Steps 1–7.30; do not repeat them or receive stack restore,
dynamic VLA allocation, VLA GEP, or any other row unless 794 itself selected
that exact one.  The classification does not choose that row because the
baseline identifies no receiver-ready candidate.

## Deferred / unassigned boundary

No standalone Raw-BIR receiver idea is created by this queue: every current
unreceived family lacks a named accepted handoff.  Module/global metadata and
each residual receiver packet remain deferred to their listed first owner.  If
an existing owner discovers a distinct family outside its stated scope, it must
create a separate single-owner successor rather than extend 734 or this queue.
