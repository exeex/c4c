# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.32
Current Step Title: Receive selected AMD64 aggregate VA-arg overflow authority

## Just Finished

- Step 7.32: imported the one selected non-volatile AMD64 SysV aggregate
  overflow `va_arg` memcpy carrier into a typed Raw-BIR receipt, with exact
  local/derived-source, payload type, and positive i64-size checks.

## Suggested Next

- Supervisor: inspect this coherent Step 7.32 packet and choose the next
  in-scope receiver packet.

## Watchouts

- All other memcpy and memory/VA rows remain fail-closed. The receipt does not
  lower to target code or generalize the producer contract.

## Proof

- `cmake --build --preset default` and
  `ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_interface$'`
  passed; `test_after.log` preserves the output.
