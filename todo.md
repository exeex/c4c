Status: Active
Source Idea Path: ideas/open/242_inline_asm_memory_address_constraints.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Harden Tests And Regression Scope

# Current Packet

## Just Finished

Step 5 hardened the inline-asm memory/address coverage so the supported route is
proven as a structured prepared-home rule rather than a single memory fixture.

Concrete work completed:

- Prepared-printer fail-closed coverage now includes invalid local-slot
  authority for both `m` and `p`, proving unsupported structured address forms
  remain rejected instead of inferred from operand spelling.
- AArch64 dispatch coverage now proves both memory-input and address-input
  prepared pointer-value homes select into structured `MemoryOperand` payloads
  with concrete register homes and distinct offsets/sizes.
- AArch64 machine-printer coverage now directly prints both selected `m` and
  selected `p` operands from structured payloads, while existing selected records
  without payloads remain fail-closed.

## Suggested Next

Next packet: close handling is blocked until the close-time regression guard can
pass over canonical logs. No additional executor code packet is suggested from
this Step 5 slice.

## Watchouts

- Do not infer memory/address operands from inline-asm template text or final
  assembly text.
- The supported rule remains intentionally limited to structured pointer-value
  addresses whose base value matches the operand value and whose prepared home
  is a concrete GPR register.
- Local-slot and other non-pointer `MemoryAddress` forms still record
  `unsupported_operandN_memory_address_selection` or
  `unsupported_operandN_address_selection` during preparation.

## Proof

Passed:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log`

Proof log: `test_after.log`

Close gate checked:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`

Result: close rejected because canonical `test_before.log` and `test_after.log`
both report 139/139 passing backend tests, and the guard requires a strictly
increased pass count for acceptance.
