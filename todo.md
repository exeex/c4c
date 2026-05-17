Status: Active
Source Idea Path: ideas/open/259_aarch64_cast_ops_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Move cast spelling and diagnostics

# Current Packet

## Just Finished

Completed Step 4 by moving scalar cast and scalar conversion printer spelling
and diagnostics from `machine_printer.cpp` into `cast_ops.cpp`/`cast_ops.hpp`.
`machine_printer.cpp` now delegates scalar cast printing to `cast_ops` while
retaining generic scalar routing and ALU fallback behavior.

## Suggested Next

Delegate the next Step 4 packet or supervisor review scope to decide whether
any remaining cast-specific machine-printer helper boundaries should move.

## Watchouts

- Preserve scalar cast behavior; this is ownership redistribution only.
- Do not expand cast semantics or rewrite expectations to claim progress.
- Generic `machine_printer.cpp` register naming helpers still serve non-cast
  printer paths; this packet intentionally duplicated the cast-local spelling
  helpers rather than changing those shared routes.
- Keep `cast_ops.md` until its valid durable content has been reconciled.

## Proof

Ran exactly:

```bash
{ cmake --build build --target c4c_backend backend_aarch64_machine_printer_test backend_aarch64_scalar_cast_records_test backend_aarch64_prepared_scalar_cast_records_test backend_aarch64_scalar_record_contract_test && ctest --test-dir build -R '^(backend_aarch64_machine_printer|backend_aarch64_scalar_cast_records|backend_aarch64_prepared_scalar_cast_records|backend_aarch64_scalar_record_contract)$' --output-on-failure; } > test_after.log 2>&1
```

Result: passed. `test_after.log` contains 4/4 selected tests passed.
