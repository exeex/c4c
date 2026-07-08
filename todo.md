Status: Active
Source Idea Path: ideas/open/602_bir_local_memory_load_semantics.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Locate the Missing BIR Load Fact

# Current Packet

## Just Finished

Completed Step 2: traced the selected load rows to the BIR local-memory load
producer and narrowed the Step 3 implementation target. No implementation,
tests, expectations, unsupported markers, persistent allowlists,
runtime/timeout/accounting behavior, `plan.md`, source ideas, or handoff docs
were touched.

Observed load-family failure signature:

- The selected load rows all stop before prepared/RV64 handoff with
  `[RV64_C4C_OBJ_COMPILE_FAIL]`, `backend object route requires semantic
  lir_to_bir lowering before the prepared object handoff`, and
  `latest function failure: semantic lir_to_bir function '<fn>' failed in load
  local-memory semantic family`.
- Current per-case logs confirm the representative functions:
  `src/20041124-1.c` -> `main`, `src/20011008-3.c` ->
  `__db_txnlist_lsnadd`, `src/20000706-4.c` -> `bar`,
  `src/20010129-1.c` -> `foo`, and `src/920625-1.c` -> `main`.
- `build/c4cll --dump-hir --target riscv64-linux-gnu` succeeds for the probed
  rows, while `--dump-bir --target riscv64-linux-gnu` fails with the same
  `load local-memory semantic family` note. That rules out frontend/HIR as the
  first owner and confirms the missing fact is in semantic BIR production.

BIR producer trace:

- `src/backend/bir/lir_to_bir/memory/coordinator.cpp` owns the family
  diagnostic: `BirFunctionLowerer::lower_scalar_or_local_memory_inst` dispatches
  `LirLoadOp` through `lower_memory_load_inst(...)`; when that path returns
  false, it records `load local-memory semantic family`.
- `src/backend/bir/lir_to_bir/memory/local_slots.cpp` owns the implementation
  target. `BirFunctionLowerer::lower_memory_load_inst(...)` attempts global
  provenance loads, dynamic local aggregate loads, pointer-provenance loads,
  local-slot loads, and dynamic pointer-array loads before returning false.
- The narrow local-memory load helpers are
  `try_lower_local_slot_load(...)`,
  `try_lower_nonpointer_local_slot_load(...)`, and
  `try_lower_tracked_local_pointer_slot_load(...)`. Their live facts are the
  local pointer/slot maps in `src/backend/bir/lir_to_bir/lowering.hpp`:
  `local_pointer_slots_`, `local_slot_types_`, `local_address_slots_`,
  `local_slot_address_slots_`, `local_slot_pointer_values_`,
  `pointer_value_addresses_`, `local_scalar_slot_values_`,
  `loaded_local_scalar_immediates_`, and
  `loaded_local_integer_pointer_values_`.
- The missing BIR load fact is therefore not a prepared/RV64 consumer fact. It
  is the absence or rejection of an explicit semantic source-memory fact for a
  `LirLoadOp`: load identity plus result type, source pointer/local slot or
  pointer-provenance identity, object layout/range/size/alignment, and a
  compatible scalar destination value. The current rows include different C
  shapes, so the repair must target the producer contract rather than case
  names.

Guard ownership ruled out for this packet:

- Store rows stay owned by `lower_memory_store_inst(...)` and the `store
  local-memory semantic family`.
- GEP/address rows stay owned by `lower_memory_gep_inst(...)` and the `gep
  local-memory semantic family`.
- Alloca rows stay owned by `lower_local_memory_alloca_inst(...)` and the
  `alloca local-memory semantic family`.
- Prepared authority rows such as destination fan-in/prealloc classification
  happen after semantic BIR exists and are not the first stop for these load
  rows.
- RV64/MIR rows such as `unsupported_local_memory_access`,
  `unsupported_terminator_fragment`, and `unsupported_move_bundle_target_shape`
  are downstream consumer failures and must not be used to paper over missing
  BIR load facts.

## Suggested Next

Execute Step 3 with a narrow code-change packet in
`src/backend/bir/lir_to_bir/memory/local_slots.cpp`, plus any required
declaration adjustment in `src/backend/bir/lir_to_bir/lowering.hpp`: admit one
ordinary local-memory load shape by teaching `lower_memory_load_inst(...)` /
`try_lower_local_slot_load(...)` to publish or consume the explicit local
source-memory fact instead of returning false. Start with scalar local-slot or
pointer-provenance load production; keep aggregate/member, va_arg/byval,
dynamic aggregate, store, GEP, alloca, prepared, and RV64 consumer behavior as
guarded/fail-closed unless the delegated Step 3 packet explicitly owns a
subshape.

## Watchouts

- Keep named cases as probes only, not match keys.
- Do not route this through RV64 target inference, expectation edits,
  unsupported markers, allowlists, timeout/runtime handling, or accounting.
- The old load signature to clear is the first-owner producer stop:
  `[RV64_C4C_OBJ_COMPILE_FAIL]` with `backend object route requires semantic
  lir_to_bir lowering before the prepared object handoff` and
  `failed in load local-memory semantic family`.
- Guard rows must remain honestly classified unless a later lifecycle route
  owns them. In particular, store/GEP/alloca producer rows, prepared
  destination fan-in, prepared/RV64 local-memory access, and RV64/MIR consumer
  fragments are not proof of load-family progress.
- `src/20041124-1.c` is complex/aggregate-adjacent and `src/920625-1.c` is
  va_arg/aggregate-adjacent, so they are breadth probes, not the first
  implementation shape. Do not make Step 3 depend on those source names or on
  final RV64 assembly behavior.

## Proof

Delegated proof command for the Step 2 `todo.md` update was run and saved in
`test_after.log`:

```sh
rg 'Step 2|missing BIR load fact|load local-memory|lir_to_bir|BIR producer|implementation target|guard rows' todo.md
```

Additional evidence-only probes used `c4c-clang-tool-ccdb` for AST-backed
symbol lookup in the BIR lowering files and `build/c4cll --dump-hir` /
`--dump-bir` on representative rows. The BIR dump probe intentionally fails
with the same producer diagnostic before prepared handoff; no code or
persistent allowlist was changed.
