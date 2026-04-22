# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 2.2
Current Step Title: Migrate Canonical Call And Return Families
Plan Review Counter: 3 / 6
# Current Packet

## Just Finished

Completed step 2.2’s generic operand/result bridge cleanup by moving
`const_as_imm32`, `emit_alloca_addr_to`, `operand_to_reg`, `operand_to_rax`,
and `store_rax_*` out of `lowering/call_lowering.cpp` and into the compiled
`core/x86_codegen_output.cpp` seam. `call_lowering.cpp` now keeps only
call-family ABI selection and lowering behavior instead of owning shared
operand/result helper bodies.

## Suggested Next

Keep step 2.2 on canonical ownership cleanup by classifying any remaining
shared helper bodies that still exist only in the dormant reviewed draft path
(`shared_call_support.cpp`) and migrate the next genuinely non-call-generic
piece through a compiled seam without widening into prepared-route or ABI
policy work.

## Watchouts

- The backend target still does not compile `shared_call_support.cpp` or
  `mod.cpp`; step 2.2 cleanup must continue using the reviewed compiled seams
  rather than re-linking the legacy owners.
- `memory.cpp` still contains a dormant `emit_alloca_addr_to` body; this packet
  intentionally left that legacy owner untouched because it is outside the
  compiled seam and outside owned-file authority.
- Keep `calls.cpp` and `returns.cpp` non-owning, and do not widen this route
  into ABI policy changes, prepared-route admission logic, or return-lowering
  work.

## Proof

Step 2.2 generic operand/result bridge cleanup on 2026-04-22:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed (`106/106`). Canonical log paths: `test_before.log`,
`test_after.log`
