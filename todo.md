Status: Active
Source Idea Path: ideas/open/328_rv64_prepared_edge_publication_module_flow.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce And Localize The Missing Publication Move

# Current Packet

## Just Finished

Step 1 `Reproduce And Localize The Missing Publication Move` completed. The
focused proof reproduces `backend_riscv_prepared_edge_publication` failing with
`RISC-V prepared module should emit a register edge move`; the final prepared
RV64 module text is missing `mv a1, a0`.

The focused fixture builds shared prepared edge-publication authority for
`join_regs` and separately proves the helper can render `mv a1, a0` from the
shared lookup. The skip is in
`src/backend/mir/riscv/codegen/prepared_module_emit.cpp::emit_prepared_module_text`:
the successful
`append_simple_prepared_bir_function_asm(out, module, &lookups, *function_it)`
branch immediately `continue`s before the later
`lookups.edge_publications.publications` loop can call
`append_edge_publication_move_instruction(...)`.

## Suggested Next

Start Step 2 from `plan.md`: repair RV64 prepared module/function scheduling so
available shared `edge_publications` are appended through
`append_edge_publication_move_instruction(...)` even when the simple prepared
function emitter succeeds.

## Watchouts

- Do not special-case fixture names, value ids, registers, or the literal
  expected move string as the implementation mechanism.
- Do not rediscover edge moves by scanning BIR predecessor/successor blocks.
- Preserve fail-closed behavior for unsupported homes and missing shared lookup
  authority.
- `append_simple_prepared_bir_function_asm(...)` currently returns true for the
  minimal `join_regs` function shape, so a repair in `emit_prepared_module_text`
  must avoid losing the existing fallback behavior for functions the simple
  emitter cannot handle.
- Keep idea 327 out of scope.

## Proof

Ran:
`set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^backend_riscv_prepared_edge_publication$' --output-on-failure) > test_after.log 2>&1`

Result: build completed and the focused test failed as expected, reproducing the
current missing final assembly move. Proof log: `test_after.log`.
