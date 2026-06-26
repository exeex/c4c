Status: Active
Source Idea Path: ideas/open/403_prepared_i16_formal_abi_publication.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Publish GPR Bank Facts For Direct I16 Formals

# Current Packet

## Just Finished

Step 2 repaired producer-side direct `I16` formal ABI register-bank
publication.

Changed files:

- `src/backend/prealloc/target_register_profile.cpp`: added `I16` to the
  shared scalar integer `register_bank_from_type()` rule so
  `register_bank_from_arg_abi()` and ABI register placements publish
  `PreparedRegisterBank::Gpr` for integer `I16` arguments.
- `src/backend/prealloc/storage_plans.cpp`: added `I16` to the storage-plan
  scalar integer bank helper so direct formal storage records agree with the
  prepared home/ABI register bank.
- `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`:
  added a focused RV64 direct `I16` formal fixture that checks `%p.x`/`%p.y`
  homes publish GPR target identities for `a0`/`a1` and storage publishes
  `encoding=register bank=gpr reg=a0/a1`.
- `todo.md`: recorded Step 2 proof and handoff.

Fresh `src/divmod-1.c` prepared outcome:

- `div2`, `div4`, `mod2`, and `mod4` direct `I16` formals no longer show
  `encoding=register bank=none reg=aN`.
- The fresh dump now shows the formerly blocking lines as
  `encoding=register bank=gpr reg=a0/a1`, including:
  `div2` `%p.x`, `div4` `%p.x`/`%p.y`, `mod2` `%p.x`, and
  `mod4` `%p.x`/`%p.y`.
- Existing corrected 407 caller-side same-module `I16` facts remain present:
  frame-slot call arguments still publish
  `dest_placement=gpr:call_argument#N/w1`, `dest_reg=aN`, `dest_bank=gpr`,
  and `missing_frame_slot_arg_publication=yes`.
- Old caller-side residual shapes remain absent in the fresh dump:
  no `source_encoding=frame_slot ... dest_bank=none`, no
  `call_arg_stack_to_stack`, and no `placement=none:call_argument`.

## Suggested Next

Step 3 should run the divmod representative proof and classify the next
residual, if any. This packet did not implement RV64 instruction-fragment
lowering; if `src/divmod-1.c` still fails after the producer facts are fixed,
route the fresh failure back to 395 or another owner based on the new
diagnostic.

## Watchouts

- Do not infer GPR bank inside RV64 object emission from `reg=a0`, parameter
  order, or formal type.
- Do not reopen 407 unless fresh prepared dumps show the old same-module
  caller-side `i16` producer regression.
- Caller-side local `I16` bank patches in regalloc call binding/move code were
  left in place, per packet guidance to avoid broad cleanup. They may now be
  partly redundant, but removing them is not needed for this slice.
- `storage_plans.cpp` has its own local scalar bank helper; Step 2 updated it
  because storage facts otherwise stayed bankless even after ABI placement
  identities were repaired.

## Proof

Exact command run:

```sh
cmake --build --preset default && mkdir -p build/agent_state/403_reopen_step2_dumps && build/c4cll --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/divmod-1.c > build/agent_state/403_reopen_step2_dumps/divmod-1.prepared.txt 2> build/agent_state/403_reopen_step2_dumps/divmod-1.prepared.err && rg -n 'bir\.sext i16|storage %p\.[xy].*encoding=register|bank=none reg=a[0-9]|bank=gpr reg=a[0-9]|home %p\.[xy]|source_encoding=frame_slot.*dest_bank=none|call_arg_stack_to_stack|missing_frame_slot_arg_publication' build/agent_state/403_reopen_step2_dumps/divmod-1.prepared.txt build/agent_state/403_reopen_step2_dumps/divmod-1.prepared.err && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result: command completed successfully. The fresh prepared dump/grep shows
the `I16` formal storage facts now publish `bank=gpr reg=aN`, and 407
caller-side frame-slot call-argument facts remain complete.
`test_after.log` is preserved and green with `100% tests passed, 0 tests
failed out of 326`.

Additional focused pre-proof run:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepare_frame_stack_call_contract$'
```

Result: passed, `1/1` tests green.
