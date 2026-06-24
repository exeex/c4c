Status: Active
Source Idea Path: ideas/open/341_rv64_vector_register_inline_asm_constraints_stage2.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Validate Stage 2 Against Stage 1 And Negative Cases

# Current Packet

## Just Finished

Step 5: Validate Stage 2 Against Stage 1 And Negative Cases completed.

Ran the supervisor-delegated backend build/subset proof against the Stage 2
vector inline-asm constraint work. The backend subset still includes and passes
the focused RV64 object-emission coverage for scalar `.insn` behavior,
positional scalar substitution, vector `VR`/`VRM2`/`VRM4` base-register
substitution, tied vector substitution, malformed constraint rejection,
allocation overlap rejection, grouped alignment rejection, and impossible
allocation rejection.

The only failing backend test in the refreshed proof is the supervisor-accepted
unrelated baseline failure
`backend_codegen_route_riscv64_pointer_typed_select_publication`, which matches
`test_before.log`. No new backend failures were introduced.

## Suggested Next

Ask the plan owner whether the runbook should close or split any remaining
follow-up now that Step 5 validation is complete.

## Watchouts

- Do not reactivate the parked Stage 1 child unless lifecycle rules explicitly
  require it.
- Keep Stage 2 focused on `VR`, `VRM2`, and `VRM4`; EV `.insn.d`,
  named-operands, masks, and consteval asm strings are later children.
- Treat testcase-overfit, expectation weakening, and raw-string constraint
  acceptance as route failures.
- The existing RV64 object helper
  `rv64_register_number_for_inline_asm_operand` still hard-codes `=r`, `+r`,
  `r`, and numeric tied GPR operands. Keep future EV `.insn.d` object encoding
  separate from this scalar fail-closed path.

## Proof

Ran exact delegated proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`

Result: build succeeded; backend CTest completed with only the supervisor-known
unrelated baseline failure
`backend_codegen_route_riscv64_pointer_typed_select_publication`. No new
backend failures were present, matching `test_before.log`. Proof log:
`test_after.log`.
