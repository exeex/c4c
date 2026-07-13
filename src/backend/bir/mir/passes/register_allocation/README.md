# Register Allocation

Status: scaffold. Assigns virtual registers or value fragments to physical
registers and spill slots under target constraints. Its inputs include selected
machine operations, call clobbers, inline-asm constraints, and MIR liveness.

Legacy coverage: `prealloc/regalloc.*` and every file under
`prealloc/regalloc/`, including classification, intervals, values, assignments,
value homes, pointer carriers, calls, phi copies, stack slots, special carriers,
and runtime helpers.

The audit also includes `regalloc_placement_identity.*`; placement identity must
be an MIR allocation product, never a BIR name/index fallback.
