Status: Active
Source Idea Path: ideas/open/228_aarch64_module_phoenix_drafts_to_implementation.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Implement One Lowering Family At A Time

# Current Packet

## Just Finished

Step 5: Implement One Lowering Family At A Time removed the AArch64 codegen
records bucket files from the live build. `records.hpp` and `records.cpp` were
deleted, the backend source list now builds `codegen/instruction.cpp`, and
module/printer/test includes now use `codegen/instruction.hpp` instead of
`codegen/records.hpp`.

The slice does not resurrect the legacy module emitter, add a replacement
records shim, downgrade test expectations, or change selected AArch64 lowering
behavior.

## Suggested Next

Supervisor can review this deletion/reconnect slice, then choose whether the
next packet should further split the remaining shared instruction surface into
domain-owned `alu`, `memory`, `comparison`, `return`, and `call` translation
units or proceed to the next bounded AArch64 lowering family.

## Watchouts

- `InstructionRecord` and related temporary type names still survive in the new
  shared instruction files; fully renaming those types would be a separate
  larger packet.
- Historical/docs references to `records.cpp/.hpp` outside the owned live
  code/test/build paths were left untouched.
- No domain wrapper headers were added because that would recreate a shim under
  another spelling without actually moving ownership.

## Proof

Ran:
`cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_|backend_cli_aarch64_asm_external_return_zero_smoke|backend_cli_aarch64_asm_external_return_add_smoke|backend_cli_aarch64_asm_external_return_add_sub_chain_smoke'`

Result: passed; focused subset ran 30/30 tests. Proof output is preserved in
`test_after.log`.
