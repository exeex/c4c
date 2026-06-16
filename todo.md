Status: Active
Source Idea Path: ideas/open/284_c_aggregate_function_pointer_call_ir_type_repair.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Locate The Aggregate Function-Pointer Boundary

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from
`ideas/open/284_c_aggregate_function_pointer_call_ir_type_repair.md`.

## Suggested Next

Execute Step 1: reproduce
`llvm_gcc_c_torture_src_struct_ret_1_c`, inspect the invalid aggregate/scalar
call argument boundary, and identify the owning lowering helper for repair.

## Watchouts

- Do not special-case `struct-ret-1.c` or generated symbol names.
- Do not weaken the gcc torture harness, expectations, or unsupported
  classification.
- Keep C++ dependent cast work and AArch64 fp128/vararg crash work out of this
  plan.

## Proof

Lifecycle-only activation; no build or CTest proof required.
