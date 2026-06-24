Status: Active
Source Idea Path: ideas/open/341_rv64_vector_register_inline_asm_constraints_stage2.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Vector Constraint Classification

# Current Packet

## Just Finished

Step 2: Add Vector Constraint Classification completed.

Implemented real inline-asm register-class metadata for `VR`, `=VR`, `+VR`,
`VRM2`, `=VRM2`, `+VRM2`, `VRM4`, `=VRM4`, and `+VRM4`. Supported vector
tokens now carry `InlineAsmRegisterClass::Vector` and group widths 1, 2, or 4
through BIR metadata, prepared inline-asm operands, prepared carrier validation,
and pre-regalloc `PreparedRegisterGroupOverride` publication. RV64 prepared
home identity now recognizes `v0` through `v31`, and tied vector operands are
validated against vector register identity rather than the old scalar-only GPR
check.

Malformed vector-looking tokens such as `VRM8`, `=VRM3`, and `+VRM` now emit
explicit `unsupported_vector_constraintN:<token>` facts instead of being treated
as opaque accepted strings. Existing scalar `r`, `=r`, numeric tied inputs,
integer immediates, memory/address operands, and clobber behavior was preserved.

Focused tests added BIR metadata coverage for supported/malformed vector
constraints and prepared-printer coverage for RV64 vector group overrides,
vector homes, grouped carrier facts, and tied vector authority. No object
encoding, substitution, EV `.insn.d`, named operand, mask, or consteval work was
included.

## Suggested Next

Delegate Step 3 to implement and harden grouped vector allocation/overlap
rules for inline asm operands. The next packet should prove legal `VR`,
`VRM2`, and `VRM4` bases, full group reservation, no untied overlap, allowed
tied reuse, and impossible-allocation diagnostics before substitution or object
encoding work.

## Watchouts

- Do not reactivate the parked Stage 1 child unless lifecycle rules explicitly
  require it.
- Keep Stage 2 focused on `VR`, `VRM2`, and `VRM4`; EV `.insn.d`,
  named-operands, masks, and consteval asm strings are later children.
- Treat testcase-overfit, expectation weakening, and raw-string constraint
  acceptance as route failures.
- The pre-regalloc override hook is intentionally in
  `src/backend/prealloc/prealloc.cpp` so regalloc sees inline-asm vector
  class/width facts before assignment; keep later packets from moving this back
  to carrier-only publication.
- `+VR*` currently maps to a register output with an input arg index so prepare
  sees read-write value use. Step 3 should add allocator-focused overlap tests
  around this shape.
- Current RV64 object helper
  `rv64_register_number_for_inline_asm_operand` hard-codes `=r`, `+r`, `r`,
  and numeric tied GPR operands. Vector substitution should not weaken these
  scalar fail-closed checks.
- The existing RV64 vector span helper splits caller/callee pools into
  `v0..v15` and `v16..v31`; the Stage 2 contract says allocation is over
  `v0..v31`, so the implementation packet should decide whether this policy is
  acceptable or needs a target-profile adjustment.

## Proof

Ran exact delegated proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`

Result: build succeeded; backend CTest completed with only the known unrelated
baseline failure `backend_codegen_route_riscv64_pointer_typed_select_publication`.
No new backend failures were present. Proof log: `test_after.log`.
