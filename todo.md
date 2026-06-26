Status: Active
Source Idea Path: ideas/open/389_rv64_va_start_destination_va_list_address_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Capture The Prepared `va_start` Destination Address Boundary

# Current Packet

## Just Finished

Step 1 captured the prepared/BIR/object boundary for the RV64 `va_start`
destination `va_list` address runtime failure in
`tests/c/external/gcc_torture/src/va-arg-13.c`.

Prior runtime boundary:

- The prior representative run still reports
  `[RV64_BACKEND_RUNTIME_MISMATCH]` with `clang_exit=0` and
  `c4c_exit=Segmentation fault`.
- `build/agent_state/388_step5_va-arg-13.readelf.log` shows no
  `llvm.va_end.p0` relocation or symbol, so the active failure is past the
  idea 388 `va_end` boundary.

BIR positions:

- In focused semantic BIR for `test`, the first `va_start` is
  `bir.call void llvm.va_start.p0(ptr %lv.state.8)` at entry instruction 3.
- The second `va_start` is the same call shape at entry instruction 12.
- Both calls target the addressable local `state.ap` storage path represented
  by `%lv.state.8`.

Prepared facts:

- Prepared call plans preserve both calls as
  `wrapper_kind=direct_extern_fixed_arity`, `callee=llvm.va_start.p0`,
  one GPR argument.
- The prepared variadic entry plan has `helpers=[va_start]`.
- Helper operands are complete enough to identify both destinations:
  - block 0 inst 3:
    `dst_va_list=%lv.state.8:stack_slot:slot=#13:offset=72`
    and `dst_va_list_addr=%lv.state.8:register:reg=s1`
  - block 0 inst 12:
    `dst_va_list=%lv.state.8:stack_slot:slot=#13:offset=72`
    and `dst_va_list_addr=%lv.state.8:register:reg=s1`
- Ordinary value homes publish `%lv.state.8` / value id 10 as
  `kind=register reg=s1`. Storage plans also show `%lv.state.8 value_id=10`
  encoded as a register in callee-saved `s1`.
- Stack layout separately publishes object #13
  `__rv64_variadic_va_start_destination_6` at slot #13 offset 72, plus the
  overflow-area base slot #12 at the same stack offset. The destination stack
  slot exists, but the address of that slot is not visibly materialized into
  the address home before use.
- Move bundles before `va_start` inst 3 and inst 12 only publish call-argument
  moves from value id 10 to `a0`; they do not publish a value/home move or
  address-materialization move that establishes `s1 = &slot#13`.

Object evidence:

- In `test`, the first `va_start` object sequence computes the overflow-area
  pointer into `t1` with `addi t1,sp,72`, then immediately emits
  `sd t1,0(s1)` at object offset `0x7c`.
- The second `va_start` repeats the same store at object offset `0xac`.
- `s1` is saved/restored as callee-saved state, but there is no preceding
  instruction in `test` that materializes the destination `va_list` address
  into `s1` before either store.

Missing or ambiguous fact:

- Prepared data provides both the destination stack slot and an address home
  register, but the object route lacks an explicit, consumed publication that
  makes `dst_va_list_addr` valid before `fragment_for_prepared_variadic_va_start`
  stores through it.
- The missing route is therefore destination-address materialization or
  publication for the prepared `dst_va_list_addr`, not a missing `va_start`
  helper record.

Likely owner:

- RV64 object emission for prepared `va_start`, specifically the route that
  consumes `dst_va_list_addr` before storing the overflow-area pointer.
- The next packet should decide whether the owner is a narrow object-emission
  materialization inside the `va_start` fragment from the prepared stack slot,
  or an earlier prepared move/address publication fact that must be emitted
  before the helper. It should stay semantic and not hard-code `s1`, slot 13,
  offset 72, or `va-arg-13.c`.

## Suggested Next

Execute Step 2: classify the route for making `dst_va_list_addr` valid before
the RV64 `va_start` store: either object-emission materialization from explicit
prepared destination stack-slot facts or a stricter fail-closed requirement for
prepared publication/move facts.

## Watchouts

- Keep idea 386 frame-slot-address call arguments and idea 388 `va_end` lowering closed.
- Keep idea 387 same-module sret calls out of scope.
- Do not hard-code `va-arg-13.c`, function `test`, register `s1`, or literal object offsets.
- Do not broaden this packet into aggregate `va_arg`, `va_copy`, or unrelated variadic helper rewrites.

## Proof

Ran the supervisor-selected Step 1 evidence command:

```bash
mkdir -p build/agent_state && { echo 'Step 1 va_start destination address evidence for idea 389'; echo '--- prior runtime boundary ---'; cat build/agent_state/388_step5_va-arg-13.case.log; echo '--- readelf va_end absence / relocs ---'; rg -n 'llvm\.va_end|Relocation|Symbol table|dummy|abort|exit|test' build/agent_state/388_step5_va-arg-13.readelf.log || true; echo '--- objdump va_start stores ---'; rg -n '<test>|Ltest_entry|sd[[:space:]]+t1,0\(s1\)|s1|addi|sd|ld' build/agent_state/388_step5_va-arg-13.objdump.log | sed -n '1,260p'; echo '--- dump-bir focused test ---'; ./build/c4cll --dump-bir --target riscv64-linux-gnu --mir-focus-function test tests/c/external/gcc_torture/src/va-arg-13.c > build/agent_state/389_step1_va_arg_13.bir.log; rg -n 'llvm\.va_start|llvm\.va_end|dummy|call_arg_source|lv\.state|t7|t14' build/agent_state/389_step1_va_arg_13.bir.log | sed -n '1,260p'; echo '--- dump-prepared-bir focused test ---'; ./build/c4cll --dump-prepared-bir --target riscv64-linux-gnu --mir-focus-function test tests/c/external/gcc_torture/src/va-arg-13.c > build/agent_state/389_step1_va_arg_13.prepared_bir.log; rg -n 'llvm\.va_start|va_start|dst_va_list|destination_va_list|destination_va_list_address|helper|call block_index=0 inst_index=(3|12)|move_bundle|storage %lv\.state|storage %lv\.state\.8|object #|slot #|saved_register|prepared\.func @test' build/agent_state/389_step1_va_arg_13.prepared_bir.log | sed -n '1,320p'; } > test_after.log 2>&1
```

Result: passed.

Proof log: `test_after.log`

Evidence logs:

- `build/agent_state/389_step1_va_arg_13.bir.log`
- `build/agent_state/389_step1_va_arg_13.prepared_bir.log`
