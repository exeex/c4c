# Current Packet

Status: Complete
Source Idea Path: ideas/open/303_rv64_runtime_defined_global_scalar_memory_foundation.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Lower Initialized Global i32 Loads

## Just Finished

Completed `plan.md` Step 4: lowered initialized scalar i32 `LoadGlobalInst`
through the RV64 prepared global-memory owner using the prepared
`GlobalSymbol` memory-access record and semantic global label emission.

Implemented load behavior:

- Initialized scalar i32 global loads resolve the prepared memory access by
  block/instruction/result, require default address space, nonvolatile
  direct-global addressing, zero byte offset, size 4, alignment at least 4, and
  base-plus-offset availability.
- The emitted code materializes the semantic global label with `lla` and loads
  the i32 value with `lw` into the prepared result register.
- The raw BIR `LoadGlobalInst` may omit `align_bytes`; the prepared access
  record remains the alignment authority for this route.
- Only initialized scalar i32 globals are lowered in this packet. Zero-init
  global loads and global stores still fail closed for later steps.
- Registered only `backend_rv64_runtime_global_load` after the repo RV64
  runtime harness executed `global_load.c` under qemu with expected exit code
  `11`.

## Suggested Next

Execute `plan.md` Step 5 by extending the same RV64 prepared global-memory load
path to default-zero scalar i32 globals and registering
`global_load_zero_init.c` only after the real RV64 asm route executes under
qemu with expected exit code `0`.

## Watchouts

- Step 5 should remove or relax only the initialized-global restriction for the
  existing load path; keep the same prepared access and simple-defined-global
  gates.
- Do not register `global_store.c` or add store lowering in Step 5.
- The raw load instruction's `align_bytes` can be `0`; keep relying on the
  prepared memory-access record for size/alignment proof.
- The prepared access symbol id belongs to the prepared name table, while the
  storage label may come from the BIR module global. Preserve that distinction
  when sharing helpers.
- Reject testcase-shaped shortcuts, fallback acceptance, and scope expansion
  into arrays, aggregates, pointer initializers, extern globals, TLS, or
  GOT-heavy work.

## Proof

Ran the supervisor-selected proof successfully; output is captured in
`test_after.log`:

```sh
cmake --build --preset default && ctest --test-dir build -R '^backend_rv64_runtime' --output-on-failure && ./build/c4cll --codegen asm --target riscv64-unknown-linux-gnu tests/backend/case/global_load.c -o /tmp/c4c_global_load.s && rg 'g_counter:|\.word[[:space:]]+11|lla[[:space:]]+[a-z0-9]+,[[:space:]]*g_counter|lw[[:space:]]+[a-z0-9]+,[[:space:]]*0\([a-z0-9]+\)' /tmp/c4c_global_load.s
```

Result: build passed, all 24 matching `backend_rv64_runtime` tests passed
including `backend_rv64_runtime_global_load`, and the generated RV64 assembly
showed `g_counter:`, `.word 11`, `lla t0, g_counter`, and `lw t0, 0(t0)`.
