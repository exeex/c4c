# Current Packet

Status: Active
Source Idea Path: ideas/open/783_lir_native_vaarg_operand_result_seam_decomposition.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Enumerate the three native vaarg structural seams

## Just Finished

- Plan Step 2 inventory (no implementation or contract change):
  - AArch64 GP: `emit_aarch64_vaarg_gp_src_ptr`
    (`src/codegen/lir/hir_to_lir/call/vaarg.cpp:47`) reads `gr_top` through
    va_list field 1, emits `LirGepOp{reg_addr, "i8", gr_top, ..., offs}`, and
    joins `reg_addr` with the overflow `stack_ptr` in `src_ptr`'s `LirPhiOp`.
    The consumer boundary is the scalar/aggregate caller in
    `emit_rval_payload` (load or memcpy); proposed probe target is a new
    structural AArch64 sibling of
    `test_aarch64_scalar_stdarg_preserves_structured_va_list` in
    `tests/frontend/frontend_lir_call_type_ref_test.cpp`.
  - AArch64 FP/alignment: `emit_aarch64_vaarg_fp_src_ptr`
    (`vaarg.cpp:97`) reads `vr_top` through field 2 and produces `reg_addr` by
    i8 GEP; its stack arm reads field 0, conditionally uses the ptrmask call
    (alignment > 8) or ptr-to-int/add/and/int-to-ptr chain, then joins
    `reg_addr` with `aligned_stack_ptr` in `src_ptr`'s `LirPhiOp`. The
    consumer boundary is the FP/FP128 load in `emit_rval_payload`; proposed
    probe target is the same new AArch64 structural sibling, covering a
    floating and an alignment-requiring payload separately.
  - AMD64 register/stack: `emit_amd64_va_arg`
    (`src/codegen/lir/hir_to_lir/call/vaarg_amd64.cpp:68`) obtains va_list
    access pointers through `load_amd64_va_list_ptrs`, branches on register
    availability, receives `reg_value` from the register helper and
    `stack_value` from `emit_amd64_va_arg_from_overflow`, and returns their
    `LirPhiOp` result. The consumer boundary is that returned payload value;
    proposed probe target is a new non-semantic AMD64 structural fixture next
    to the existing vaarg helper test.

## Suggested Next

- Plan Step 3: add only the three focused frontend-LIR structural probes from
  this inventory, preserving the observed helper boundaries and recording the
  current unasserted links as tests rather than changing lowering behavior.

## Watchouts

- `tests/backend/case/` is not the probe location for this frontend-LIR
  authority work; do not substitute backend or rendered-output assertions.
- Do not modify PHI carrier/verification or absorb Raw-BIR/importer, backend,
  target lowering, MIR, emission, generic migration, or text recovery.
- Current gap: the existing AArch64 test prints LLVM text only to establish
  structured va_list presence, while the existing vaarg authority test forces
  semantic AMD64 vaarg lowering. Neither asserts the GP `gr_top`/`reg_addr`,
  FP alignment, nor non-semantic AMD64 register/stack helper path. These are
  gaps, not newly assumed contracts.
- The next probes must inspect LIR operands, result IDs, and branch/join
  relationships; do not turn this inventory into rendered-text or named-case
  matching.

## Proof

- Passed fresh: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^frontend_lir_call_type_ref$'` (1/1 tests passed).
  The supervisor-selected focused structural proof is sufficient for this
  inventory packet. Full command output: `test_after.log`.
