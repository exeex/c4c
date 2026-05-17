Status: Active
Source Idea Path: ideas/open/259_aarch64_cast_ops_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Move cast spelling and diagnostics

# Current Packet

## Just Finished

Completed Step 4 F128 helper-boundary review without changing output. The
remaining F128 cast helper spelling/diagnostics branch stays in
`machine_printer.cpp` for now because it is part of the shared
`print_f128_runtime_helper(...)` helper boundary: the cast branch reuses the
same provenance checks, full-width q-register marshal/unmarshal printers, and
scalar ABI move printers as the F128 arithmetic and comparison helper branches.
Moving only the cast branch into `cast_ops` would require either duplicating
that broad F128 runtime-helper infrastructure or exposing shared helper
printer internals outside the current machine-printer boundary.

## Suggested Next

Supervisor review or plan-owner closure check for Step 4. If the larger F128
runtime-helper printer boundary is split later, move the cast branch together
with the shared helper infrastructure rather than as an isolated cast-only
packet.

## Watchouts

- Preserve scalar cast behavior; this is ownership redistribution only.
- Do not expand cast semantics or rewrite expectations to claim progress.
- Generic `machine_printer.cpp` register naming helpers and F128 helper
  marshal printers still serve non-cast printer paths; keep the F128 cast
  helper branch there until a broader F128 runtime-helper printer split is
  explicitly owned.
- Keep `cast_ops.md` until its valid durable content has been reconciled.

## Proof

Ran exactly:

```bash
{ cmake --build build --target c4c_backend backend_aarch64_machine_printer_test backend_aarch64_instruction_dispatch_test backend_aarch64_target_instruction_records_test && ctest --test-dir build -R '^(backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records)$' --output-on-failure; } > test_after.log 2>&1
```

Result: passed. `test_after.log` contains 3/3 selected tests passed.
