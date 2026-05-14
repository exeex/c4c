# Current Packet

Status: Active
Source Idea Path: ideas/open/218_aarch64_structured_asm_encoder_linker_contract.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Add Focused Guards If A Live Risk Exists

## Just Finished

Completed Step 6 from `plan.md`: inspected live AArch64 routes for an
accidental bridge from `machine_printer.cpp` output into parser/encoder/object
or linker code and recorded the guard decision.

Guard decision:

- No live route currently sends printed `.s` from
  `print_machine_instruction_nodes(...)` or the AArch64 backend assembly route
  into the in-tree AArch64 `parse_asm`, `assemble`, `encode_instruction`,
  object, or linker entry points.
- The live AArch64 `--codegen asm` path builds prepared machine nodes, calls
  the terminal machine printer, and returns text to the caller; the assembler,
  encoder, object, and linker surfaces are declarations/staged compatibility
  boundaries with no live internal call chain from that printer output.
- Because there is no live accidental parser/encoder/object/linker route, no
  focused guard test is needed for this plan.

## Suggested Next

Execute Step 7 validation and handoff for the contract slice, preserving
`test_after.log` as the latest executor proof.

## Watchouts

- Preserve the separation between terminal printer output, external assembler
  input, structured encoder/object records, and linker object inputs.
- The delegated route search included missing `src/c4cll` and `src/driver`
  paths; `rg` reported those missing paths, but the command intentionally used
  `|| true` for that search group and still proved the docs/todo vocabulary.
- If a future implementation wires AArch64 printed `.s` into an in-tree
  assembler/parser/encoder/object/linker path, add the narrow focused guard at
  that call chain instead of guarding the current terminal text return.
- Do not edit the source idea for routine execution details; record packet
  state here instead.

## Proof

Delegated proof completed successfully and wrote `test_after.log`:

```bash
bash -lc 'set -o pipefail; { rg -n "print_machine_instruction_nodes|machine_printer|parse_asm|assemble_module|assemble\(|encode_instruction|load_first_static_input_objects|link_first_static_executable" src/backend src/c4cll src/driver tests || true; rg -n "no live route|guard test|focused guard|printed \.s|parser/encoder/object/linker" src/backend/mir/aarch64/MACHINE_INSTRUCTION_NODE_CONTRACT.md src/backend/mir/aarch64/codegen/records.md todo.md; } > test_after.log 2>&1; test -s test_after.log'
```

Proof result: route search plus contract vocabulary confirms the no live route
decision and the current no focused guard test requirement.
