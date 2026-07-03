Status: Active
Source Idea Path: ideas/open/571_rv64_inline_asm_carrier_lowering.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement RV64 Inline Asm Carrier Object Emission

# Current Packet

## Just Finished

Steps 2-3: Add Focused Carrier Tests / Implement RV64 Inline Asm Carrier Object
Emission is complete.

Added focused backend object-emission coverage for:

- a complete no-result side-effecting `llvm.inline_asm` carrier with the
  `~{memory}` clobber, asserting it emits through the carrier path as an empty
  instruction fragment followed by the function `ret`
- fail-closed no-result memory-clobber shapes, asserting they use the precise
  `unsupported_inline_asm_fragment` diagnostic
- the `pr51933`-style symbol-address `imr,imr,imr,~{memory}` unsupported
  constraint shape, asserting it rejects with an inline-asm-specific diagnostic
  and does not reach the generic `unsupported_instruction_fragment` fallback

Implemented the smallest RV64 object-emission path in
`fragment_for_prepared_call`: complete empty-template, no-result, side-effecting
memory-clobber carriers now produce an empty encoded fragment. Unsupported
inline asm call carriers now receive the precise
`unsupported_inline_asm_fragment: RV64 object route requires a complete supported
inline-asm carrier` diagnostic before the generic unsupported-instruction
fallback.

## Suggested Next

Execute Step 4 by rerunning the four RV64 gcc_torture carrier representatives
(`src/20071211-1.c`, `src/pr51933.c`, `src/pr56982.c`, `src/pr78438.c`) through
the object route and saving compact evidence under
`build/agent_state/571_rv64_inline_asm_carrier_lowering/`.

## Watchouts

- This plan is limited to RV64 inline asm carrier calls.
- Do not implement general call ABI lowering, pointer arithmetic, select,
  floating-point binary, or branch-published phi lowering here.
- Do not add filename-specific matching for `src/20071211-1.c`,
  `src/pr51933.c`, `src/pr56982.c`, or `src/pr78438.c`.
- Do not change expectations, unsupported markers, allowlists, or runtime
  comparison behavior.
- `src/pr51933.c` is a symbol-address and unsupported-constraint diagnostic
  shape, not a generic symbol-address call-lowering target for this plan.
- Keep unsupported inline asm forms on precise inline-asm-specific diagnostics
  instead of the old generic fallback; do not treat `imr` as supported by
  choosing one named representative-specific interpretation.
- The new positive emission path intentionally supports only the complete
  no-result empty-template `~{memory}` carrier; `.insn r`, `.insn.d`, and empty
  tied scalar GPR inline asm remain on their existing paths.

## Proof

Run:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$') > test_after.log 2>&1`

Result: passed. Test subset: `backend_riscv_object_emission`. Log path:
`test_after.log`.
