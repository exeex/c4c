Status: Active
Source Idea Path: ideas/open/365_rv64_va_start_overflow_base_state_production.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Overflow Base-State Producer Boundary

# Current Packet

## Just Finished

Completed `plan.md` Step 1 audit for
`ideas/open/365_rv64_va_start_overflow_base_state_production.md`.

Inspection results:

- Representative log
  `build/rv64_gcc_c_torture_backend/src_920908-1.c/case.log` still fails at
  `unsupported_variadic_helper_lowering: RV64 va_start helper requires prepared overflow-area initial base state`.
- Prepared carrier fields are defined in
  `src/backend/prealloc/variadic.hpp` as
  `PreparedVariadicEntryOverflowArea::{required, base_slot_id,
  base_stack_offset_bytes, align_bytes}`. The prepared printer already emits
  these as `overflow_area required=... base_slot=... base_stack_offset=...`.
- RV64 ABI fact production is in
  `src/backend/prealloc/variadic_entry_plans.cpp`:
  `populate_rv64_variadic_entry_abi_facts` sets `overflow_area.required`,
  `overflow_area.align_bytes=8`, and the single pointer-sized
  `OverflowArgArea` va_list field, then the RV64 branch removes
  `target_abi.variadic_entry_state` and `target_abi.va_list_layout` and
  publishes helper resources plus `va_start` operand homes. It does not publish
  `overflow_area.base_slot_id` or `base_stack_offset_bytes`.
- AAPCS64 has the comparable semantic producer pattern:
  `populate_aapcs64_variadic_entry_abi_facts` first marks
  `overflow_area.base_slot_id` and `overflow_area.base_stack_offset_bytes`
  missing, then `attach_aapcs64_variadic_entry_storage_authority` appends or
  finds a dedicated frame slot with source kind
  `aapcs64_variadic_overflow_area_base`, assigns the slot id and stack offset,
  and removes both missing facts.
- The RV64 object consumer is in
  `src/backend/mir/riscv/codegen/object_emission.cpp`:
  `rv64_variadic_va_start_runtime_state_diagnostic` requires both overflow
  base fields before materialization, and
  `rv64_variadic_va_start_materialization_diagnostic` also requires a
  pointer-sized overflow va_list field, a GPR destination va_list address, and
  a 12-bit signed immediate for `base_stack_offset_bytes`.
- Focused coverage already proves the boundary:
  `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`
  asserts AAPCS64 frame-slot authority for the overflow base and asserts RV64
  has helper homes/layout but no overflow-base contract yet;
  `tests/backend/bir/backend_prepared_printer_test.cpp` checks the printable
  AAPCS64 storage facts and RV64 helper-family facts; and
  `tests/backend/mir/backend_riscv_object_emission_test.cpp` rejects a
  prepared RV64 `va_start` without base state while materializing when a test
  fixture injects explicit base state.

## Suggested Next

Step 2 implementation packet: add an RV64 storage-authority producer in
`src/backend/prealloc/variadic_entry_plans.cpp`, mirroring the AAPCS64 semantic
producer shape but using RV64 source-kind names such as
`rv64_variadic_overflow_area_base`. It should create or find a zero-sized,
8-byte-aligned frame slot for supportable RV64 helper-bearing variadic entries,
publish `overflow_area.base_slot_id` and `base_stack_offset_bytes`, and leave
object emission gated on those explicit prepared facts. Update the focused BIR
contract/printer tests to assert RV64 base-slot and base-offset publication.

Suggested narrow proof command:

```sh
cmake --build build --target backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_riscv_object_emission_test && ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_riscv_object_emission)$' --output-on-failure
```

## Watchouts

- Treat `src/920908-1.c` as a representative signal, not an implementation key.
- Do not reopen idea-364 object-emission helper lowering except for narrow
  consumer adjustments required by a finalized producer contract.
- Do not broaden into external variadic call lowering, general parameter-home
  expansion, scalar `va_arg`, `va_copy`, or aggregate `va_arg`.
- Progress must publish explicit prepared overflow-area base state; a
  diagnostic rename, expectation rewrite, or object-emission guess is not
  enough.
- The current struct stores `base_stack_offset_bytes` as `std::size_t`, while
  the plan asks Step 2 to define a signed-offset convention. Step 2 should
  either document that the current prepared-frame offset is non-negative from
  the stable stack base for this producer, or deliberately change the contract
  type/sign convention with matching focused coverage.
- Helper-free RV64 variadic entries are still rejected by object admission even
  if the base state is present; keep that separate from the helper-bearing
  `va_start` producer work.

## Proof

Audit-only packet; no build required because only `todo.md` changed.

Inspection commands run:

- `git status --short`
- `sed -n` reads of `AGENTS.md`, `plan.md`, `todo.md`,
  `ideas/open/365_rv64_va_start_overflow_base_state_production.md`, and
  `ideas/closed/364_rv64_va_start_helper_lowering.md`
- `rg -n "overflow_area|base_stack_offset|base_slot|va_start helper|prepared overflow|Variadic" ...`
- `sed -n` reads of the focused producer, printer, RV64 consumer, test, and
  representative-log ranges named in the packet

Final formatting proof:

```sh
git diff --check -- todo.md
```
