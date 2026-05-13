Status: Active
Source Idea Path: ideas/open/216_aarch64_machine_node_asm_printer_external_smoke.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Add External Toolchain Smoke Tests

# Current Packet

## Just Finished

Step 4 added external-toolchain smoke coverage for the public
`c4cll --codegen asm --target aarch64-linux-gnu input.c -o out.s` route.

Completed work:
- Added the smallest real source-to-selected-machine-node path for AArch64
  return terminators: source `.c` now lowers through HIR -> LIR -> semantic BIR
  -> prepared BIR -> AArch64 module build -> `FunctionRecord::machine_nodes`
  -> machine printer for `return 0`.
- The public AArch64 `.s` route now emits a GNU assembler translation unit with
  `.text`, function symbol metadata, selected node text, and `.note.GNU-stack`.
- Added an external smoke that starts from `.c`, invokes public `c4cll`, checks
  the generated `.s`, assembles it with `aarch64-linux-gnu-as`, links it with
  `clang`, and runs the resulting executable on native AArch64 hosts.
- Kept unsupported/no-node diagnostic coverage by moving the no-selected-node
  CLI case to a global-only source that still reaches the printer boundary but
  has no selected function machine nodes.

## Suggested Next

Supervisor should review and commit the Step 4 slice, then hand the active plan
to plan-owner for close/deactivate/split decision if all todo items are now
complete.

## Watchouts

- The `.s` printer is output-only; do not parse printed assembly back into
  backend semantics.
- Do not bypass BIR/prepared BIR, AArch64 target MIR, or machine-node selection
  when serving the public CLI route.
- Do not implement an internal assembler, encoder, object writer, linker, or
  `.s` parser in this plan.
- Keep smoke proof rooted in the public `c4cll` route and the external
  toolchain when available.
- Scalar records currently do not carry a destination register operand, and
  load records currently do not carry a destination register operand; the
  printer intentionally rejects those instead of guessing from value ids.
- The selected return printer is intentionally narrow: it prints void returns
  and small non-negative integer immediates to `w0`/`x0` followed by `ret`.
- The public route still emits function-level assembly only for functions with
  selected printable machine nodes; data-only inputs remain the no-node
  diagnostic path.

## Proof

Delegated proof passed and was written to `test_after.log`:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_cli_aarch64_asm|backend_aarch64_machine_printer|backend_aarch64_prepared_frame_control|backend_aarch64_target_instruction_records)') > test_after.log 2>&1`

Result: 5 tests matched and passed, including
`backend_cli_aarch64_asm_external_return_zero_smoke` and
`backend_cli_aarch64_asm_no_machine_nodes_fails`.
