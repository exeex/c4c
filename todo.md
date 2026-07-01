Status: Active
Source Idea Path: ideas/open/443_rv64_prepared_value_operand_materialization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Trace RV64 Value Consumer Selection

# Current Packet

## Just Finished

Completed `plan.md` Step 2 trace for the RV64 store-value consumer path that
loses the prepared `%t18` value while materializing global `@c`.

Specific files and functions:

- `src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp`
  - `emit_riscv_simple_store_global` is the smallest direct store-global
    implementation surface. It resolves the prepared `StoreGlobalInst`, checks
    `simple_i32_global_access_for`, finds the prepared global, copies
    `store.value`, materializes the global address, and emits the final `sw`.
  - `emit_riscv_direct_global_address_materialization` is the shared direct
    global-address materializer and writes the requested destination register
    with `lla`.
- `src/backend/mir/riscv/codegen/prepared_scalar_emit.cpp`
  - `emit_move_to_register` is the prepared value consumer helper used by the
    store path. For a named value such as `%t18`, it consults
    `prepared_register_for_value`; prepared evidence maps `%t18`/`value_id=16`
    to `t0`, so a safe store path must copy from `t0` to a distinct store-value
    scratch before any `@c` address materialization writes `t0`.
- `src/backend/mir/riscv/codegen/prepared_emit_context.cpp`
  - `prepared_register_for_value` and `prepared_value_home_for` are the
    prepared-home lookup boundary used by `emit_move_to_register`; the trace did
    not find a missing producer/home fact for `%t18`.
- `src/backend/mir/riscv/codegen/prepared_function_emit.cpp`
  - the instruction loop dispatches `bir::StoreGlobalInst` directly to
    `emit_riscv_simple_store_global` with the current instruction context.

Data-flow rule for Step 3:

- Treat the prepared store source as authoritative: `StoreGlobalInst.value`
  `%t18` has prepared home `register t0` and must be consumed into a register
  that will not be overwritten by address materialization.
- The bad object shows the opposite ordering at
  `build/agent_state/443_step1_prepared_value_evidence/pr81503.c4c-objdump.txt:78-84`:
  `lla`/PC-relative materialization writes `t0` with the address of `@c`,
  then `mv t1,t0` copies the address, and `sw t1,0(t0)` stores the address
  instead of the prepared `%t18` value. That is how `%t18`/`t0` is lost.
- The smallest likely Step 3 surface is
  `prepared_global_memory_emit.cpp::emit_riscv_simple_store_global`, with
  `prepared_scalar_emit.cpp::emit_move_to_register` and
  `prepared_emit_context.cpp::prepared_register_for_value` as support
  boundaries. The implementation rule should force `store.value` consumption
  before global-address materialization, or choose an address register that
  cannot clobber the prepared source when the source is already in `t0`.

## Suggested Next

Implement Step 3 in the store-global path first: preserve the prepared store
source across address materialization for direct global stores, then regenerate
focused `pr81503.c` object/objdump evidence to prove the emitted order stores
`%t18` rather than the `@c` address.

## Watchouts

- Current checked-out `emit_riscv_simple_store_global` source already appears
  to append `emit_move_to_register(..., "t1", ..., store.value)` before
  `lla t0, label`, but the preserved Step 1 object shows the bad post-`lla`
  `mv t1,t0` order. Treat that as a source/object evidence mismatch that Step
  3 should resolve with fresh generated assembly or object proof before
  deciding whether the source needs a code change or the artifact came from a
  stale/non-selected path.
- `./build/c4cll --codegen asm --target riscv64-linux-gnu
  tests/c/external/gcc_torture/src/pr81503.c` currently fails with
  `riscv prepared module emitter does not support this prepared global storage
  layout`; the focused object route from Step 1 remains the useful proof route.
- Consume prepared value authority directly; do not infer producer facts from
  testcase names, raw source shape, register accidents, or qemu exit codes.
- Do not change expectations, allowlists, unsupported markers, runtime
  comparison, or pass/fail accounting.
- Keep frame-slot call-argument publication in the separate idea `444`.

## Proof

Ran the delegated proof command exactly:

`bash -lc 'git diff --check -- todo.md > test_after.log 2>&1'`

Proof passed. `test_after.log` is preserved.
