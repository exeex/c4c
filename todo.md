# Current Packet

Status: Active
Source Idea Path: ideas/open/784_lir_native_vaarg_operand_carrier_foundation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory carrier surfaces and prove structural-probe feasibility

## Just Finished

- 784 Plan Step 1 carrier inventory (no code or test change):
  - AArch64 GP: `LirLoadOp{gr_top, "ptr", gr_top_ptr}` is the producer-role;
    `fresh_tmp()` makes `gr_top` a string, then
    `LirGepOp{reg_addr, "i8", gr_top, ...}` is its immediate structural
    consumer and `LirPhiOp{{reg_addr, reg label}, {stack_ptr, stack label}}`
    is the result boundary. The source has no native operand/value ID here.
  - AArch64 FP/alignment: `LirLoadOp{vr_top, "ptr", vr_top_ptr}` and the
    alignment-arm `stack_ptr`/`aligned_stack_ptr` values are string-built;
    the i8 `reg_addr` GEP and alignment operations consume them, and
    `LirPhiOp{{reg_addr, reg label}, {aligned_stack_ptr, stack label}}` is the
    immediate join/result boundary. It likewise has no native value transport.
  - AMD64 register/stack: the string results from
    `emit_amd64_va_arg_from_registers` and
    `emit_amd64_va_arg_from_overflow` are the producer roles; the final
    `LirPhiOp{{reg_value, reg label}, {stack_value, stack label}}` is their
    only shared immediate structural consumer/result boundary.
  - Shared smallest carrier candidate: existing `fresh_value(FnCtx&)` already
    creates a current-function `LirOperand::ssa` with a module-allocated
    `LirValueId`; use it for the listed result/operand transport. For the join,
    change only the incoming *value* from `std::string` to `LirOperand` while
    retaining the existing string label/predecessor presentation.
  - Decision: native PHI incoming value transport is indispensable. All three
    chains reach the requested consumer boundary through their PHI, and
    `LirPhiOp::incoming` currently stores both fields as strings. The printer
    renders those strings directly and the verifier checks only PHI result,
    type, and non-emptiness; value-only transport can preserve labels and does
    not require predecessor/edge identity or verification work owned by 751.

## Suggested Next

- Plan Step 2: implement only the selected current-function `LirOperand`
  transport and value-only PHI incoming carrier, preserving string rendering
  compatibility and excluding verifier and CFG/predecessor work.

## Watchouts

- Keep the decision limited to native operand/result transport. PHI
  predecessor/edge identity and verification remain owned by 751.
- Do not use text recovery or absorb Raw-BIR/importer, backend, target lowering,
  MIR, emission, broad generic-expression redesign, or 782 helper fields.
- The exact successor boundary for any predecessor/edge identity or PHI
  incoming-verification expansion is 751; this Step 1 evidence supports only
  value transport with unchanged label strings.

## Proof

- Passed fresh: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^frontend_lir_call_type_ref$'` (1/1 tests passed).
  Full command output: `test_after.log`. No code or test change was made.
