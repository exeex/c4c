Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Populate semantics and compatibility rendering in HIR-to-LIR

# Current Packet

## Just Finished

- Corrected Plan Step 2 read/write memory lowering to evaluate each output
  lvalue once, reuse its cached pointer for compatibility rendering, and load
  the structured old value from that pointer while keeping a distinct new
  semantic result.

## Suggested Next

- Execute Plan Step 3 by transactionally mapping structured LIR operands and
  results into generic BIR SSA edges without consulting compatibility text.

## Watchouts

- Keep non-scalar output addresses cached across output, tied-input, and
  semantic-old-value construction; calling `emit_lval` or `emit_rval_id` again
  can duplicate side effects in member/index/call-based lvalues.
- Multi-output/memory compatibility lowering still prints through pointer
  arguments, while semantic results use distinct ordinary SSA names. Step 3
  must consume `ordinary_inputs`/`ordinary_results`, never `args_str`,
  `result`, or `ret_type`, as semantic authority.
- The existing unrelated dirty README changes remain outside this slice.

## Proof

- Passed: `cmake --preset default && cmake --build --preset default && ctest
  --test-dir build -j --output-on-failure -R
  '^(frontend_hir_tests|backend_lir_to_bir_interface|inline_asm_aarch64_simple)$'
  > test_after.log 2>&1`.
- Canonical proof log: `test_after.log` (3/3 tests passed).
