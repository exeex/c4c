# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 2.2
Current Step Title: Migrate Canonical Call And Return Families
Plan Review Counter: 2 / 6
# Current Packet

## Just Finished

Completed step 2.2’s core-seam cleanup packet by moving generic x86
output/state plumbing out of `lowering/call_lowering.cpp` and into the
compiled `core/x86_codegen_output.cpp` seam. `X86CodegenState`,
`X86CodegenOutput`, and the small shared register/output helpers that the
canonical lowerers consume now live under `core/`, while `call_lowering.cpp`
keeps only call-family behavior plus the temporary operand/result helpers it
still owns for this rebuild stage.

## Suggested Next

Keep step 2.2 on canonical ownership cleanup by classifying the remaining
temporary operand/result helper bridge in `lowering/call_lowering.cpp`
(`const_as_imm32`, `emit_alloca_addr_to`, `operand_to_reg`, `store_rax_*`) and
move any non-call-generic bodies to the reviewed shared owner without widening
into prepared-route or ABI-policy work.

## Watchouts

- The backend target still does not compile `shared_call_support.cpp` or
  `mod.cpp`; step 2.2 cleanup must continue using the reviewed compiled seams
  rather than re-linking the legacy owners.
- `call_lowering.cpp` still carries temporary generic operand/result helpers
  because they were part of the earlier support bridge; treat those as the next
  classification target, not as long-term call ownership.
- Keep `calls.cpp` and `returns.cpp` non-owning, and do not widen this route
  into ABI policy changes, prepared-route admission logic, or return-lowering
  work.

## Proof

Step 2.2 core output/state seam cleanup on 2026-04-22:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed (`106/106`). Canonical log paths: `test_before.log`,
`test_after.log`
