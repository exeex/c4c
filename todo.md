Status: Active
Source Idea Path: ideas/open/228_aarch64_module_phoenix_drafts_to_implementation.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Implement One Lowering Family At A Time

# Current Packet

## Just Finished

Step 5: Implement One Lowering Family At A Time moved the implemented AArch64
scalar ALU instruction lowering surface out of
`module/instruction_lowering.cpp` and into new compiled `codegen/alu.cpp`, with
the active API and per-block scalar state declared by `codegen/alu.hpp`.

`module/instruction_lowering.cpp` now dispatches scalar instructions through
the codegen ALU API. Return lowering still lives in the module layer for this
packet and consumes the emitted scalar register state through the exposed ALU
state accessor.

## Suggested Next

Supervisor can review this scalar ALU ownership move, then choose whether the
next packet should move another bounded lowering family out of the module layer
or split return lowering into a domain-owned translation unit.

## Watchouts

- `codegen/alu.hpp` depends on `module/module.hpp` because the public scalar
  API still consumes module lowering context and diagnostics; moving those
  shared context types would be a separate packet.
- Return lowering intentionally remains in `module/instruction_lowering.cpp`
  for this packet, but its reuse of ALU-produced registers no longer reads a
  module-owned scalar state map directly.
- No tests or expectations were changed.

## Proof

Ran:
`cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_instruction_dispatch|backend_aarch64_return_lowering|backend_cli_aarch64_asm_external_return_add_smoke|backend_cli_aarch64_asm_external_return_add_sub_chain_smoke'`

Result: passed; focused subset ran 4/4 tests after the build. Proof output is
preserved in `test_after.log`.
