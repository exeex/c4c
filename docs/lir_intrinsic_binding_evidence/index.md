# LIR Intrinsic And Inline-Assembly Binding Evidence

This directory answers the `instruction.intrinsic-binding-evidence` routing key
from the accepted 812/813 evidence handoff.

- [01_intrinsic_inline_asm_binding_route.md](01_intrinsic_inline_asm_binding_route.md)

## Summary

`LirInlineAsmValueBinding` provides a native binding route for selected
ordinary inline-assembly values: value identity, type, role, and constraint
index are structured fields, while templates, constraints, and clobbers remain
opaque payload. The verifier checks binding roles, ordering, result uniqueness,
read/write pairing, and downstream type use. Raw-BIR import consumes selected
structured inline-asm rows without parsing assembly text.

Several builtin/intrinsic-like rows already have bounded selected receiver
routes, including accepted integer builtin chains and selected memory/VA/VLA
authority rows. This evidence does not prove a complete binding inventory for
every builtin helper form, so it does not authorize a direct 734 receiver
handoff.

Return relation:

- Future singular residual inline-asm or instruction binding handoffs belong
  first to 796.
- Verifier/dispatch/printer migration for already-published native facts
  belongs first to 846.
- 734 and 797 remain downstream until 796 or 846 accepts an exact handoff.
