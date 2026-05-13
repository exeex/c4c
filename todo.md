Status: Active
Source Idea Path: ideas/open/216_aarch64_machine_node_asm_printer_external_smoke.md
Source Plan Path: plan.md
Current Step ID: Step 6
Current Step Title: Add Focused Proof And Handoff Notes

# Current Packet

## Just Finished

Step 6 completed the focused proof and handoff notes for the AArch64
machine-node ASM printer route.

Completed work:
- Ran a fresh default build plus the supervisor-selected focused AArch64
  printer, CLI route, prepared frame/control, target instruction record, and
  external-toolchain smoke subset.
- Confirmed the public route command shape needed by idea 217:
  `c4cll --codegen asm --target aarch64-linux-gnu input.c -o out.s`.
- Confirmed the external AArch64 smoke test was available in this environment;
  the host processor is `aarch64`, so the generated binary execution path was
  not skipped.
- No proof gap requiring focused test edits was found.

## Suggested Next

Supervisor should review and commit the completed AArch64 ASM printer route
slice, then hand the active plan to plan-owner for close/deactivate/split
decision because the runbook steps are exhausted.

## Watchouts

- The `.s` printer is output-only; do not parse printed assembly back into
  backend semantics.
- Do not bypass BIR/prepared BIR, AArch64 target MIR, or machine-node selection
  when serving the public CLI route.
- Do not implement an internal assembler, encoder, object writer, linker, or
  `.s` parser in this plan.
- The selected return printer remains intentionally narrow: it prints void
  returns and small non-negative integer immediates to `w0`/`x0` followed by
  `ret`.
- Idea 217 should document the implemented public command shape exactly as
  `c4cll --codegen asm --target aarch64-linux-gnu input.c -o out.s`.

## Proof

Delegated Step 6 proof was written to `test_after.log`:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_cli_aarch64_asm|backend_aarch64_machine_printer|backend_aarch64_prepared_frame_control|backend_aarch64_target_instruction_records)') > test_after.log 2>&1`

Result: pass.

CTest subset passed 5/5:
- `backend_aarch64_target_instruction_records`
- `backend_aarch64_prepared_frame_control`
- `backend_aarch64_machine_printer`
- `backend_cli_aarch64_asm_no_machine_nodes_fails`
- `backend_cli_aarch64_asm_external_return_zero_smoke`

Environment limitations: none for this focused proof. `clang` and the AArch64
assembler were configured, and local `uname -m` reported `aarch64`, so the
external smoke covered emit, assemble, link, and run.
