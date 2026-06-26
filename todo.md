Status: Active
Source Idea Path: ideas/open/389_rv64_va_start_destination_va_list_address_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify The Publication Owner

# Current Packet

## Just Finished

Step 2 classified the owner and guard facts for making the RV64 `va_start`
`dst_va_list_addr` valid before the helper stores through it.

Chosen owner route:

- The next implementation should materialize `dst_va_list_addr` inside
  `fragment_for_prepared_variadic_va_start` from explicit prepared
  destination stack-slot facts.
- Do not require a new earlier publication or move fact for this narrow route:
  Step 1 evidence shows the before-call move bundles for inst 3 and inst 12
  only publish call-argument moves, while the prepared helper operand already
  pairs `destination_va_list` with `destination_va_list_address`.
- The route belongs in
  `src/backend/mir/riscv/codegen/object_emission.cpp`, using the existing
  `fragment_for_prepared_variadic_va_start`,
  `rv64_variadic_va_start_materialization_diagnostic`, and
  `prepared_stack_slot_home_offset` family of helpers.

Source facts from Step 1:

- Prepared calls at block 0 inst 3 and block 0 inst 12 are
  `wrapper_kind=direct_extern_fixed_arity`, `callee=llvm.va_start.p0`,
  indirect `no`, with one prepared GPR argument.
- Prepared helper operands for both calls are complete:
  `dst_va_list=%lv.state.8:stack_slot:slot=#13:offset=72` and
  `dst_va_list_addr=%lv.state.8:register:reg=s1`.
- Stack layout publishes the destination frame slot and the overflow-area base
  offset; object output currently computes the overflow-area pointer and then
  emits `sd t1,0(s1)` without first materializing `s1 = sp + dst_slot_offset`.

Guard facts for the next implementation:

- Only handle prepared variadic helper kind `VaStart` recognized from the
  call, with a matching entry plan and matching helper operand homes for the
  exact block/instruction index.
- Require complete `va_start` operand homes:
  `destination_va_list` and `destination_va_list_address` both present.
- Require `destination_va_list` to be a prepared stack-slot home whose slot id,
  offset, size, and alignment resolve against `prepared.stack_layout` and the
  current `stack_frame_bytes`.
- Require `destination_va_list_address` to be a prepared GPR home.
- Require the existing RV64 `va_start` runtime/materialization checks to pass:
  helper-free RV64 entry contract, supported 8-byte overflow-arg-area field,
  supported overflow-area base stack offset, and signed-12-bit immediates for
  the store field offset/base forms actually emitted.
- Emit semantic frame-relative address materialization for the destination
  stack slot before the overflow-area store; do not depend on the literal
  testcase, function name, value name, register `s1`, slot #13, or offset 72.

Unsupported or malformed variants must fail closed:

- Missing helper operand homes, helper kind mismatch, non-`VaStart` helper, or
  incomplete `destination_va_list` / `destination_va_list_address` homes.
- `destination_va_list` not in a supported stack-slot home, missing slot id or
  offset, slot/layout mismatch, unsupported size/alignment, or frame offset
  outside supported RV64 immediate/materialization bounds.
- `destination_va_list_address` not in a prepared GPR home.
- Missing or unsupported RV64 variadic entry contract facts, missing
  overflow-area field/base, unsupported field size/offset, or helper variants
  such as `va_arg`, aggregate `va_arg`, or `va_copy`.
- Ordinary direct extern `llvm.va_start.p0` calls without the prepared
  variadic helper facts remain out of scope.

## Suggested Next

Execute Step 3: add focused RV64 object-emission coverage proving prepared
`va_start` materializes the destination `va_list` address from the prepared
stack-slot home before storing the overflow-area pointer, plus fail-closed
coverage for malformed homes.

## Watchouts

- Keep idea 386 frame-slot-address call arguments and idea 388 `va_end` lowering closed.
- Keep idea 387 same-module sret calls out of scope.
- Do not hard-code `va-arg-13.c`, function `test`, value `%lv.state.8`,
  register `s1`, slot #13, or literal offset 72.
- The first implementation packet should be limited to
  `src/backend/mir/riscv/codegen/object_emission.cpp`,
  `tests/backend/mir/backend_riscv_object_emission_test.cpp`, and `todo.md`.
- Do not broaden this packet into aggregate `va_arg`, `va_copy`, scalar
  `va_arg`, or unrelated variadic helper rewrites.

## Proof

Ran the supervisor-selected Step 2 classification command:

```bash
mkdir -p build/agent_state && { echo 'Step 2 va_start destination address route classification for idea 389'; echo '--- clang tool availability ---'; command -v c4c-clang-tool-ccdb || true; echo '--- object_emission variadic/helper symbols ---'; c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/riscv/codegen/object_emission.cpp build/compile_commands.json 2>/dev/null | rg 'fragment_for_prepared_variadic_va_start|rv64_variadic_va_start_materialization|destination|frame_address|prepared.*home|variadic' || true; echo '--- relevant object_emission anchors ---'; rg -n 'fragment_for_prepared_variadic_va_start|rv64_variadic_va_start_materialization_diagnostic|destination_va_list|destination_va_list_address|dst_va_list|overflow_arg_area|store_double|addi|stack_offset|PreparedVariadic|PreparedValueHome' src/backend/mir/riscv/codegen/object_emission.cpp src/backend/prealloc/variadic_entry_plans.cpp src/backend/prealloc/variadic.hpp | sed -n '1,320p'; echo '--- Step 1 prepared facts ---'; rg -n 'helper_operand kind=va_start|dst_va_list|dst_va_list_addr|storage %lv\.state\.8|object #13|slot #13|move_bundle phase=before_call .*instruction_index=(3|12)|call block_index=0 inst_index=(3|12)' build/agent_state/389_step1_va_arg_13.prepared_bir.log | sed -n '1,260p'; } > test_after.log 2>&1
```

Result: passed.

Proof log: `test_after.log`

Evidence logs:

- `build/agent_state/389_step1_va_arg_13.bir.log`
- `build/agent_state/389_step1_va_arg_13.prepared_bir.log`
