Status: Active
Source Idea Path: ideas/open/403_prepared_i16_formal_abi_publication.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Prove Divmod And Route Residuals

# Current Packet

## Just Finished

Step 3 proved the post-producer-fix `src/divmod-1.c` representative and
classified the residual state.

Representative outcome:

- `build/c4cll --target riscv64-linux-gnu
  tests/c/external/gcc_torture/src/divmod-1.c` completed with
  `c4cll_status=0`.
- `divmod-1.err` is empty; no `unsupported_instruction_fragment`,
  `unsupported`, `error`, `fatal`, or lower/object diagnostic appeared in the
  proof grep.
- The command output produced the lowered module text, including the prior
  `sext i16` functions, without failing.

Prepared fact reconfirmation:

- A Step 3 prepared dump under
  `build/agent_state/403_reopen_step3_probe/divmod-1.prepared.txt` confirms
  the direct `I16` formal bankless shape is gone.
- `div2`, `div4`, `mod2`, and `mod4` direct `I16` formals publish
  `encoding=register bank=gpr reg=a0/a1`.
- No `encoding=register bank=none reg=aN` direct formal facts were found.
- Corrected 407 caller-side same-module `I16` facts remain complete:
  frame-slot call arguments still publish
  `dest_placement=gpr:call_argument#N/w1`, `dest_reg=aN`, `dest_bank=gpr`,
  and `missing_frame_slot_arg_publication=yes`.
- Old caller-side residual shapes remain absent: no
  `source_encoding=frame_slot ... dest_bank=none`, no
  `call_arg_stack_to_stack`, and no `placement=none:call_argument`.

Classification:

- The active 403 producer-side `I16` formal ABI/home publication blocker is
  cleared for the primary representative.
- No fresh residual was exposed by this packet, so nothing routes back to 395,
  another existing open idea, or a new initiative from the Step 3 proof.
- 403 appears closure-ready subject to supervisor/plan-owner lifecycle review;
  this executor packet does not close the idea.

## Suggested Next

Supervisor should hand this to the plan owner for lifecycle review/closure of
403, or choose any broader validation it wants before closure. No same-surface
403 implementation packet is currently indicated.

## Watchouts

- Do not infer GPR bank inside RV64 object emission from `reg=a0`, parameter
  order, or formal type.
- Do not reopen 407 unless fresh prepared dumps show the old same-module
  caller-side `i16` producer regression.
- If later proof exposes a real RV64 instruction-fragment residual, route that
  through 395 or the appropriate active/open idea rather than keeping it under
  403.

## Proof

Exact command run:

```sh
cmake --build --preset default && mkdir -p build/agent_state/403_reopen_step3_probe && (build/c4cll --target riscv64-linux-gnu tests/c/external/gcc_torture/src/divmod-1.c > build/agent_state/403_reopen_step3_probe/divmod-1.out 2> build/agent_state/403_reopen_step3_probe/divmod-1.err; printf 'c4cll_status=%s\n' "$?" > build/agent_state/403_reopen_step3_probe/divmod-1.status) && cat build/agent_state/403_reopen_step3_probe/divmod-1.status && { rg -n 'unsupported_instruction_fragment|unsupported|error|fatal|bir\.sext|bank=none|bank=gpr|instruction fragment|lower' build/agent_state/403_reopen_step3_probe/divmod-1.out build/agent_state/403_reopen_step3_probe/divmod-1.err || true; } && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result: command completed successfully. `divmod-1.status` contains
`c4cll_status=0`. `test_after.log` is preserved and green with
`100% tests passed, 0 tests failed out of 326`.

Additional classification dump:

```sh
build/c4cll --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/divmod-1.c > build/agent_state/403_reopen_step3_probe/divmod-1.prepared.txt 2> build/agent_state/403_reopen_step3_probe/divmod-1.prepared.err && rg -n 'bir\.sext i16|storage %p\.[xy].*encoding=register|encoding=register bank=none reg=a[0-9]|bank=gpr reg=a[0-9]|source_encoding=frame_slot.*dest_bank=none|call_arg_stack_to_stack|placement=none:call_argument|missing_frame_slot_arg_publication' build/agent_state/403_reopen_step3_probe/divmod-1.prepared.txt build/agent_state/403_reopen_step3_probe/divmod-1.prepared.err
```

Result: direct `I16` formals publish `bank=gpr`; old bankless formal and
caller-side residual shapes are absent.
