Status: Active
Source Idea Path: ideas/open/243_inline_asm_tied_home_allocation_policy.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Numeric-Tie And Prepared-Home Surfaces

# Current Packet

## Just Finished

Step 1 inspected the numeric-tie and prepared-home route. Concrete change
surfaces are:

- `src/backend/bir/bir.hpp` and `src/backend/bir/lir_to_bir/calling.cpp`:
  numeric constraints become `InlineAsmOperandKind::TiedInput` with
  `tied_output_index`; no alias or target-home identity is represented there.
- `src/backend/prealloc/prealloc.hpp` and `src/backend/prealloc/prealloc.cpp`:
  `PreparedInlineAsmCarrier` carries operand homes plus `result_home`, but it
  has no structured tied-home/coallocation proof beyond concrete
  `PreparedValueHome::register_name` presence.
- `src/backend/mir/aarch64/codegen/dispatch.cpp`: AArch64 selection gates tied
  operands in `require_inline_asm_tied_home_agreement`; it currently requires
  both homes to be concrete registers and compares raw `register_name` strings.
- `src/backend/mir/aarch64/codegen/instruction.hpp` and
  `src/backend/mir/aarch64/codegen/machine_printer.cpp`: selected inline-asm
  operands preserve home facts, and the printer repeats the same concrete
  string-equality tied-home check before substitution.
- Existing coverage lives mainly in
  `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`,
  `tests/backend/mir/backend_aarch64_machine_printer_test.cpp`,
  `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`, and
  `tests/backend/bir/backend_prepared_printer_test.cpp`.

First alias-aware representative is blocked with current data: an AArch64 GPR
tie whose output prepared home is `w3` and tied input prepared home is `x3`
should be the same physical register, but both dispatch and printer would
diagnose a tied-home mismatch because they compare textual register names
instead of target-normalized register identity. Same-spelling concrete ties
such as `w3`/`w3` are already supported; missing, non-concrete, and mismatched
homes already fail closed.

## Suggested Next

Proceed to Step 2 by adding a structured tied-home authority surface in the
prepared inline-asm carrier path, then teach AArch64 dispatch/printer to use
target-normalized register identity for proven tied homes instead of raw
`register_name` equality.

## Watchouts

- Do not infer tied-home agreement from rendered assembly, equal textual names,
  or final machine printing.
- Keep allocator and prepared-home policy out of printer-local or
  dispatch-local shortcuts.
- Do not weaken current numeric-tie diagnostics or supported-path tests to
  claim progress.
- Preserve the existing same-spelling concrete tie path while adding alias-aware
  support; the first blocked case is `wN`/`xN`, not a new named-test shortcut.
- Prepared-home validation currently checks only register-kind presence for
  tied homes; target-invalid, class-invalid, and alias-normalization reasons
  need structured facts or explicit diagnostics before selection accepts them.

## Proof

Supervisor-selected proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log`

Proof status: passed; build was up to date and `ctest` reported 139/139
`backend_` tests passed. Proof log: `test_after.log`.
