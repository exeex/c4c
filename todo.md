# Current Packet

Status: Complete
Source Idea Path: ideas/open/303_rv64_runtime_defined_global_scalar_memory_foundation.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inspect Prepared Global Inputs

## Just Finished

Completed `plan.md` Step 1: inspected BIR and prepared-BIR output for
`global_load.c`, `global_load_zero_init.c`, and `global_store.c`.

Global/input facts:

- `global_load.c` and `global_store.c` define the same scalar global shape:
  `int g_counter = 11;`.
- `global_load_zero_init.c` defines the zero-initialized scalar shape:
  `int g_counter;`.
- The BIR/prepared-BIR dumps print function bodies but not global data records;
  source-level distinction is initialized i32 versus tentative zero i32.

Instruction and prepared metadata facts:

- `global_load.c`: `main` contains `%t0 = bir.load_global i32 @g_counter`
  followed by `bir.ret i32 %t0`.
- `global_load_zero_init.c`: prepared output is instruction-identical to
  `global_load.c` for `main`; both route through a `load_global` from
  `@g_counter`.
- `global_store.c`: `main` contains `bir.store_global @g_counter, i32 7`,
  then `%t0 = bir.load_global i32 @g_counter`, then `bir.ret i32 %t0`.
- All three prepared modules report
  `target=riscv64-unknown-linux-gnu route=semantic_bir_shared` with phases
  `legalize stack_layout liveness out_of_ssa regalloc`.
- The load result `%t0` is assigned a GPR home
  `placement=gpr:caller_saved#0/w1 reg=t0`, with a before-return move to
  `a0`.
- Each load has a prepared direct-global select-chain record with
  `source_producer=load_global`.
- Each global load/store has prepared addressing:
  `base=global_symbol symbol=g_counter offset=0 size=4 align=4
  base_plus_offset=yes range_verdict=proven_in_bounds`.
- The store case also has `store_source ... status=available
  intent=store_global_publication source=<none>` for instruction index 0.

Smallest Step 2 implementation surface:

- Add a narrow RV64 prepared global-memory owner, parallel to
  `prepared_local_memory_emit.*`, for same-module scalar i32 global data,
  `LoadGlobalInst`, and `StoreGlobalInst`.
- Route only `LoadGlobalInst`/`StoreGlobalInst` dispatches from
  `prepared_function_emit.cpp`; currently the simple prepared function emitter
  handles locals and then fails closed on unhandled instructions.
- Extend prepared module emission before `.text` so same-module initialized i32
  globals and zero-initialized i32 globals get assembler-visible storage.
- Use the existing prepared address facts as the authority: require
  `PreparedAddressBaseKind::GlobalSymbol`, `offset=0`, `size=4`, default
  address space, non-volatile access, and an in-bounds same-module non-extern
  non-TLS global before emitting `lw`/`sw`.
- Keep unsupported globals fail-closed: arrays, aggregates, pointer
  initializers, externs, TLS, non-i32 widths, missing prepared addressing, and
  non-immediate store forms should not be accepted by this slice.

## Suggested Next

Execute `plan.md` Step 2 by adding the smallest RV64 prepared global-memory
owner surface and routing prepared `LoadGlobalInst`/`StoreGlobalInst` through
it, without registering runtime cases yet.

## Watchouts

- `--dump-bir` and `--dump-prepared-bir` do not print the module global data
  records, so Step 2 should inspect `module.module.globals` in code rather than
  deriving initializer facts from the dump text alone.
- `prepared_module_emit.cpp` currently starts output with `.text`; global
  storage emission likely belongs before that text section.
- `prepared_function_emit.cpp` currently has no prepared `LoadGlobalInst` or
  `StoreGlobalInst` dispatch, so the present RV64 prepared route fails closed
  for these cases after local-memory handling.
- Do not register runtime cases until the real RV64 asm route executes them
  under qemu with expected exit codes.
- Reject testcase-shaped shortcuts, fallback acceptance, and scope expansion
  into arrays, aggregates, pointer initializers, extern globals, or GOT-heavy
  work.

## Proof

Ran the supervisor-selected proof successfully and captured output in
`test_after.log`:

```sh
cmake --build --preset default && ./build/c4cll --dump-bir --target riscv64-unknown-linux-gnu tests/backend/case/global_load.c && ./build/c4cll --dump-prepared-bir --target riscv64-unknown-linux-gnu tests/backend/case/global_load.c && ./build/c4cll --dump-bir --target riscv64-unknown-linux-gnu tests/backend/case/global_load_zero_init.c && ./build/c4cll --dump-prepared-bir --target riscv64-unknown-linux-gnu tests/backend/case/global_load_zero_init.c && ./build/c4cll --dump-bir --target riscv64-unknown-linux-gnu tests/backend/case/global_store.c && ./build/c4cll --dump-prepared-bir --target riscv64-unknown-linux-gnu tests/backend/case/global_store.c
```
