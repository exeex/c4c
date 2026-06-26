Status: Active
Source Idea Path: ideas/open/403_prepared_i16_formal_abi_publication.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reconfirm The Bankless I16 Formal Register Gap

# Current Packet

## Just Finished

Step 1 reconfirmed the bankless direct `I16` formal-register gap and traced
the producer family.

Fresh `src/divmod-1.c` prepared facts:

- `div2`: `bir.sext i16 %p.x to i32`; `%p.x` is
  `storage %p.x value_id=4 encoding=register bank=none reg=a0 width=1
  units=a0`.
- `div4`: `bir.sext i16 %p.x to i32` and `bir.sext i16 %p.y to i32`;
  `%p.x` is `bank=none reg=a0`, `%p.y` is `bank=none reg=a1`.
- `mod2`: `bir.sext i16 %p.x to i32`; `%p.x` is `bank=none reg=a0`.
- `mod4`: `bir.sext i16 %p.x to i32` and `bir.sext i16 %p.y to i32`;
  `%p.x` is `bank=none reg=a0`, `%p.y` is `bank=none reg=a1`.

Contrasting supported facts in the same dump:

- `I8` formals in `div1`, `div3`, `mod1`, and `mod3` publish
  `encoding=register bank=gpr reg=aN`.
- `I64` formals in `mod5` and `mod6` publish
  `encoding=register bank=gpr reg=a0/a1`.
- The value homes for all of these direct formals still have `kind=register`
  and `reg=aN`; the missing piece is the GPR bank/identity on the `I16`
  register home and storage surface.

Producer helper/function family to repair:

- `src/backend/prealloc/target_register_profile.cpp`:
  `register_bank_from_type()` omits `bir::TypeKind::I16`, so
  `register_bank_from_arg_abi()` returns `PreparedRegisterBank::None` for
  integer `I16` ABI arguments.
- `call_arg_destination_register_placement()` then produces a call-argument
  placement with `bank=None` for direct `I16` ABI registers.
- `src/backend/prealloc/regalloc/value_homes.cpp`:
  `classify_prepared_value_home()` uses
  `fixed_formal_abi_register_index()` plus
  `call_arg_destination_register_name()` /
  `call_arg_destination_register_placement()` /
  `target_register_identity_for_abi_register_placement()` for direct formal
  parameter homes. The bankless placement prevents a target register identity
  from being published, so downstream storage prints `bank=none reg=aN`.

Why previous focused coverage missed this surface:

- The 407-focused same-module `I16` test exercises caller-side call argument
  publication and before-call move/binding destinations. Those paths currently
  patch bankless `I16` call-argument placements locally in
  `src/backend/prealloc/regalloc.cpp` and
  `src/backend/prealloc/regalloc/call_moves.cpp`.
- It does not cover callee-side direct formal homes/storage for a function
  whose `I16` parameter is already in ABI register `aN`, so the shared
  target-register-profile omission remains visible only on formal storage.

## Suggested Next

Step 2 should repair the shared scalar ABI register-bank authority by making
direct integer `I16` ABI register placements publish `PreparedRegisterBank::Gpr`
through the existing target-register-profile/value-home path. Add a focused
prepared contract test that covers direct RV64 `I16` callee formal
home/storage facts, not just caller-side call argument moves.

## Watchouts

- Do not implement 395 RV64 opcode lowering while this producer blocker
  remains.
- Do not infer GPR bank inside RV64 object emission from `reg=a0`, parameter
  order, or formal type.
- Do not reopen 407 unless fresh prepared dumps show the old same-module
  caller-side `i16` producer regression.
- Treat the local caller-side `I16` bank patches in regalloc call binding/move
  code as evidence of the shared helper gap; do not add another parallel
  fallback for formal parameters.

## Proof

Exact command run:

```sh
cmake --build --preset default && mkdir -p build/agent_state/403_reopen_step1_dumps && build/c4cll --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/divmod-1.c > build/agent_state/403_reopen_step1_dumps/divmod-1.prepared.txt 2> build/agent_state/403_reopen_step1_dumps/divmod-1.prepared.err && rg -n 'bir\.sext i16|storage %p\.[xy].*encoding=register|bank=none reg=a[0-9]|bank=gpr reg=a[0-9]|home %p\.[xy]|source_encoding=frame_slot.*dest_bank=none|call_arg_stack_to_stack|missing_frame_slot_arg_publication' build/agent_state/403_reopen_step1_dumps/divmod-1.prepared.txt build/agent_state/403_reopen_step1_dumps/divmod-1.prepared.err && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result: command completed successfully. Build reported `ninja: no work to do`;
the prepared dump/grep reproduced the `I16` bankless formal facts and the
contrasting `I8`/`I64` GPR formal facts; `test_after.log` is preserved and
green with `100% tests passed, 0 tests failed out of 326`.
