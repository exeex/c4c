Status: Active
Source Idea Path: ideas/open/341_rv64_vector_register_inline_asm_constraints_stage2.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Implement Base-Register Substitution

# Current Packet

## Just Finished

Step 4: Implement Base-Register Substitution completed.

Added a compiled RV64 prepared-carrier substitution helper that accepts complete
positional inline-asm carriers and substitutes scalar `r`, `=r`, `+r`, numeric
tied operands, and Stage 2 vector `VR`, `VRM2`, and `VRM4` register operands
from their selected concrete register homes.

The vector path prints the prepared home register name only, so grouped
`VRM2`/`VRM4` operands substitute the allocated base vector register rather
than expanding occupied members or choosing a non-base member. Focused RV64
object-emission tests cover `%0`, `%1`, `%2`, all three vector widths, a mixed
scalar/vector template, and a numeric tied `=VRM4,0` template.

The scalar `.insn r` object encoder remains unchanged and still uses the
existing GPR-only `rv64_register_number_for_inline_asm_operand` checks. No EV
`.insn.d` object encoding, named operand support, `%c[...]` modifiers, mask
constraints, consteval strings, or broad RVV lowering was added.

## Suggested Next

Delegate Step 5 to validate Stage 2 against Stage 1 and negative cases, then
ask the plan owner whether the runbook should close or split any remaining
follow-up.

## Watchouts

- Do not reactivate the parked Stage 1 child unless lifecycle rules explicitly
  require it.
- Keep Stage 2 focused on `VR`, `VRM2`, and `VRM4`; EV `.insn.d`,
  named-operands, masks, and consteval asm strings are later children.
- Treat testcase-overfit, expectation weakening, and raw-string constraint
  acceptance as route failures.
- The compiled substitution helper lives next to the current prepared object
  helper surface because the dormant RV64 `asm_emitter.cpp` source is not part
  of the active backend target. This packet touched `object_emission.cpp` and
  `object_emission.hpp` only to expose the prepared-carrier substitution helper
  for compiled tests; it did not alter `.insn r` object encoding.
- The existing RV64 object helper
  `rv64_register_number_for_inline_asm_operand` still hard-codes `=r`, `+r`,
  `r`, and numeric tied GPR operands. Keep future EV `.insn.d` object encoding
  separate from this scalar fail-closed path.
- The existing RV64 vector span helper splits caller/callee pools into
  `v0..v15` and `v16..v31`; Step 3 proved this as the intended disjoint
  combined `v0..v31` allocator space. Do not collapse it into one overlapping
  caller pool without also unifying active caller/callee overlap tracking.

## Proof

Ran exact delegated proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`

Result: build succeeded; backend CTest completed with only the supervisor-known
unrelated baseline failure
`backend_codegen_route_riscv64_pointer_typed_select_publication`. No new
backend failures were present. Proof log: `test_after.log`.

Focused pre-proof check also passed:

`ctest --test-dir build -R '^backend_riscv_object_emission$' --output-on-failure`
