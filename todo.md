# AArch64 C-Testsuite Failure Family Inventory Todo

Status: Active
Source Idea Path: ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Defer Or Split Timeout Work

# Current Packet

Use the completed Step 3 frontend-boundary split as context. Next inspect the
23 `TIMEOUT` cases only enough to decide whether they should be deferred,
quarantined, or split into a safe focused idea. Do not implement fixes in this
umbrella plan.

## Just Finished

- Completed plan.md Step 3, "Inspect Frontend Boundary", using existing
  artifacts only.
- The classified scan has 49 `FRONTEND_FAIL` cases. They split into 35 target
  printer failures, 13 semantic `lir_to_bir` prepared-handoff failures, and 1
  bootstrap handoff rejection.
- Error wording/category counts:
  - 21 printer `compare_branch`: fused compare branch operands not printable.
  - 9 printer scalar non-add/sub opcodes: `or`/`and`/`xor` outside the
    printable add/sub subset.
  - 2 printer add/sub immediate range failures.
  - 2 printer scalar cast `sign_extend` source-shape failures.
  - 1 printer `sub` immediate-lhs/register-rhs failure.
  - 9 semantic `lir_to_bir` `gep local-memory` handoff failures.
  - 2 semantic `lir_to_bir` `store local-memory` handoff failures.
  - 2 semantic `lir_to_bir` `load local-memory` handoff failures.
  - 1 bootstrap `lir_to_bir` unsupported-shape handoff failure.
- Sampled target-printer-owned cases:
  - `00024.c`: global struct field arithmetic reaches machine-node printer,
    then fails on scalar `sub` with immediate lhs and register rhs.
  - `00027.c`: local `x | 4` reaches printer, then fails because scalar `or`
    is outside the printable add/sub subset.
  - `00030.c`: relational-call checks produce a partial `.s`, then fail on
    non-printable fused `compare_branch` operands.
  - `00077.c`: array parameter/local array checks produce a partial `.s`, then
    fail on non-printable fused `compare_branch` operands.
  - `00101.c`: empty `do while (0)` produces a partial `.s`, then fails on
    non-printable fused `compare_branch` operands.
  - `00213.c`: labels/gotos/statement expressions/switch labels fail on an
    add/sub immediate outside the plain `#imm` range; this is printer-owned
    but too compound for a first printer repair.
- Sampled prepared-handoff cases:
  - `00046.c`: anonymous nested struct/union aggregate stores fail before
    prepared-module handoff in `store local-memory`.
  - `00140.c`: struct-by-value plus pointer/variadic calls fail in function
    `f1`, `store local-memory`.
  - `00143.c`: Duff's-device array/pointer copy fails in `main`,
    `gep local-memory`.
  - `00157.c`: local array indexing for square table output fails in `main`,
    `gep local-memory`.
  - `00181.c`: Hanoi global-array printing fails in `PrintAll`,
    `gep local-memory`.
  - `00205.c`: global array of structs and indexed field printing fails in
    `main`, `gep local-memory`.
  - `00209.c`: function-pointer array declarator case fails in `f4`,
    `gep local-memory`.
  - `00216.c`: compound initializers, flexible array, nested aggregate, and
    range initializer coverage fails in `foo`, `load local-memory`.
- Classification:
  - Frontend-owned true C parse/sema gaps were not found in this sample; all
    sampled cases reached the AArch64 backend route and failed either at the
    target printer or at the backend route's semantic `lir_to_bir` admission
    gate.
  - Backend/printer-owned: scalar opcode spelling, fused compare-branch
    printing, add/sub immediate encoding, scalar cast source shaping, and
    immediate-lhs subtraction.
  - Handoff-boundary-owned: semantic BIR coverage for aggregate/array
    local-memory `gep`, `store`, and `load` shapes before the prepared-module
    handoff; these are not target-printer tasks.
- Candidate focused idea names:
  - `aarch64_backend_print_scalar_logical_ops`
  - `aarch64_backend_print_fused_compare_branch_operands`
  - `aarch64_backend_print_add_sub_immediate_materialization`
  - `aarch64_backend_print_scalar_cast_sign_extend_sources`
  - `aarch64_semantic_lir_to_bir_aggregate_array_gep_local_memory`
  - `aarch64_semantic_lir_to_bir_aggregate_store_load_local_memory`
  - `aarch64_backend_route_bootstrap_lir_to_bir_unsupported_shapes`
- Step 3 is complete enough to advance to Step 4.

Step 2 runtime-family context to keep for later focused ideas:
  - `aarch64_backend_stack_frame_alignment_for_sp_relative_locals`
  - `aarch64_backend_scalar_arithmetic_and_constant_value_lowering`
  - `aarch64_backend_loop_predicate_compare_bound_lowering`
  - `aarch64_backend_printf_string_literal_and_variadic_call_lowering`
  - `aarch64_backend_aggregate_global_address_value_materialization`

## Suggested Next

- Execute plan.md Step 4, "Defer Or Split Timeout Work": inspect the 23
  `TIMEOUT` cases from the existing scan artifacts only enough to decide
  whether timeout work should be deferred as late-stage, quarantined from the
  normal repair stream, or split into a safe focused idea if a common semantic
  owner is obvious.

## Watchouts

- Do not hand broad runtime scans to executors without timeout and stale-process
  cleanup.
- Do not count timeout/hang cases as ordinary repair packets.
- Do not change expected outputs, allowlists, unsupported classifications, or
  CTest behavior to improve counts.
- Keep printer coverage, semantic `lir_to_bir` admission, and runtime behavior
  ideas separate; the scan's `FRONTEND_FAIL` label does not mean C frontend
  ownership here.
- Avoid choosing compound integration cases (`00168.c`, `00213.c`, `00216.c`)
  as initial focused repair targets.
- The `RUNTIME_MISMATCH` samples with empty actual output are still not proof
  that `printf` alone is broken; loop predicates often prevent reaching the
  call, and the generated call sequence also lacks credible format-string
  addressing.

## Proof

Used existing artifacts only:

```bash
/tmp/c4c_aarch64_backend_scan_212.classified
/tmp/c4c_aarch64_backend_scan_212.log
tests/c/external/c-testsuite/src/{00024,00027,00030,00046,00077,00101,00140,00143,00157,00181,00205,00209,00213,00216}.c
build-aarch64-scan/c_testsuite_aarch64_backend/src/{00030,00077,00101}.c.s
```

Consistency repair proof: re-derived the 49 `FRONTEND_FAIL` buckets from
`/tmp/c4c_aarch64_backend_scan_212.log`; printer subcategories sum to 35
target-printer failures, semantic `lir_to_bir` subcategories sum to 13
prepared-handoff failures, and 1 case is a bootstrap handoff rejection. Updated
`Current Step ID` / `Current Step Title` to Step 4 after the completed Step 3
slice.

No broad CTest was run. No generated binaries were run manually. No
`test_after.log` was produced because this delegated packet was
artifact-inspection only and the proof contract explicitly requested existing
artifacts.
