Status: Active
Source Idea Path: ideas/open/307_rv64_text_only_fail_closed_output_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect the Text-Only Emission Path

# Current Packet

## Just Finished

Completed `plan.md` Step 1, "Inspect the Text-Only Emission Path": captured
the current prepared-BIR and RV64 asm facts for `src/00094.c`, identified the
global-storage pre-function gate that turns a no-storage extern declaration
into successful `.text`-only output, and recorded the Step 2 owner surface in
`build/rv64_text_only_contract_inspection.md`.

## Suggested Next

Execute Step 2 as a bounded implementation packet in
`src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp`, with
`src/backend/mir/riscv/codegen/prepared_module_emit.cpp` in review scope:
allow extern/no-storage global declarations to pass the RV64 prepared global
storage gate without emitting data, then verify `main` is emitted for
`src/00094.c`.

## Watchouts

- Do not special-case `src/00094.c`, `main`, exact source text, or c-testsuite
  case names.
- Do not broaden into string literal storage, external calls, aggregate
  globals, pointer globals, floating globals, scalar globals, libc calls, or
  full 93-case completion.
- Do not weaken expectations, mark the control unsupported, or accept empty
  `.text` as a supported result.
- The current prepared-module emitter still has no diagnostic/status channel:
  `emit_prepared_module_text` returns literal `.text` when
  `append_prepared_global_storage_asm` fails before function emission.
- Keep the Step 2 change to no-storage extern declarations only; unsupported
  storage definitions should still remain unsupported until separately owned.
- Keep scratch logs out of the repository root.

## Proof

Proof command delegated by the supervisor:

```sh
{ cmake --build --preset default && ./build/c4cll --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/c-testsuite/src/00094.c > /tmp/rv64_00094_prepared.txt && ./build/c4cll --codegen asm --target riscv64-linux-gnu tests/c/external/c-testsuite/src/00094.c -o /tmp/rv64_00094.s && test -f build/rv64_text_only_contract_inspection.md && rg -n 'prepared.func @main|\.text|owner surface|emit_prepared|RV64|00094' /tmp/rv64_00094_prepared.txt /tmp/rv64_00094.s build/rv64_text_only_contract_inspection.md; } > test_after.log 2>&1
```

Result: passed. Proof log: `test_after.log`.
