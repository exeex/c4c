Status: Active
Source Idea Path: ideas/open/334_aarch64_scalar_machine_node_operand_fact_printing.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Scalar Operand Fact Divergence

# Current Packet

## Just Finished

Split umbrella idea 295 to focused idea 334,
"AArch64 Scalar Machine-Node Operand Fact Printing", and activated the focused
runbook. The split follows the current compile/printer first bad facts in
`test_after.log`: `00164.c` scalar `mul` has incomplete printable RHS facts,
and `00214.c` scalar `add` lacks prepared frame-slot operand facts.

## Suggested Next

Execute Step 1 by localizing the exact selected-machine-node operand fact
divergence for `00164.c` and `00214.c`. Start from `test_after.log`, prepared
BIR dumps, generated artifacts, and AArch64 scalar instruction/printer records.
Record source operands, prepared homes, selected operand facts, storage widths,
and the first missing printer fact in `todo.md`.

## Watchouts

Do not reopen parked idea 316 from partial frame evidence unless localization
proves frame allocation is the current first owner. Do not implement a
filename-specific, instruction-index-specific, stack-offset-specific, or
diagnostic-string-only workaround. Keep the unrelated transient
`review/326_stdarg_byval_route_review.md` untouched.

## Proof

Lifecycle split/activation only. No build, CTest, or regression guard was run
for activation.
