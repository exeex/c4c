Status: Active
Source Idea Path: ideas/open/395_rv64_object_route_instruction_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Repeat Family Packets Until 395 Is Exhausted Or Needs Review

# Current Packet

## Just Finished

Step 4 residual classification for `src/divmod-1.c` completed without
implementation edits.

Fresh prepared/object artifacts under
`build/agent_state/395_step4_divmod_residual_*` show the first remaining
family is the same-module small-integer call-argument publication shape for
`i16` frame-slot arguments, not another scalar div/rem instruction fragment.

Evidence:
- The full fresh dump reaches `main` with early `i8` truncation and same-module
  call shapes such as `%t2 = bir.trunc i32 %t1 to i8` then
  `bir.call i32 div1(i8 %t2)`, with a complete register call plan.
- A minimized packet artifact
  `build/agent_state/395_step4_divmod_residual_i8_call.c` compiles through RV64
  object codegen.
- The nearby minimized same-family artifact
  `build/agent_state/395_step4_divmod_residual_i16_call.c` fails with the same
  `unsupported_instruction_fragment` diagnostic. Its prepared call plan has
  `wrapper_kind=same_module`, `arg index=0 value_bank=none`,
  `source_encoding=frame_slot`, `dest_bank=none`, and a before-call
  `call_arg_stack_to_stack` move.
- The full `src/divmod-1.c` dump has the same family at `div2`, `div4`,
  `mod2`, and `mod4` callsites: the `i16` trunc results are stack-slot homes
  with `bank=none` and same-module call arguments sourced from frame slots.

Classification: same-module call-argument shape, specifically prepared
small-integer `i16` frame-slot argument publication/call lowering. It appears
to remain inside idea 395 because the prepared call plan and frame-slot facts
are published; the RV64 object route lacks the target fragment for consuming
that same-module stack/none-bank argument form.

## Suggested Next

Delegate the next Step 4 implementation packet to lower prepared RV64
same-module small-integer frame-slot call arguments where the call plan exposes
`value_bank=none`, `source_encoding=frame_slot`, `dest_bank=none`, and the move
bundle records `call_arg_stack_to_stack`. Use `src/divmod-1.c` plus the
minimized `i16_call` artifact as the representative family, with the `i8_call`
artifact as the contrasting already-supported register-argument shape.

## Watchouts

- Do not treat the stale `i16 sret(size=4, align=...)` text in the prepared BIR
  signature as the primary fact; the actionable prepared call-plan facts are
  the `frame_slot`/`bank=none` same-module argument records.
- The next packet should not add filename-specific `divmod-1.c` handling. It
  should consume the prepared call-plan/move-bundle facts for this reusable
  argument family.
- If implementation discovers that `dest_bank=none` for scalar `i16`
  same-module arguments is not target-consumable by design, route that producer
  fact gap for lifecycle review instead of reconstructing ABI intent in object
  emission.
- Preserve the fixed canonical log path `test_after.log` for executor proof.

## Proof

Ran the delegated proof command exactly:

```sh
cmake --build --preset default && mkdir -p build/agent_state/395_step4_divmod_residual_dumps && printf '%s\n' 'src/divmod-1.c' > build/agent_state/395_step4_divmod_residual.allowlist.txt && { ALLOWLIST=build/agent_state/395_step4_divmod_residual.allowlist.txt STOP_ON_FAILURE=0 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/395_step4_divmod_residual_probe.log 2>&1 || true; } && rg -n 'unsupported_instruction_fragment|unsupported_stack_frame|unsupported_local_memory_access|unsupported_global_data|RV64_BACKEND_RUNTIME_MISMATCH|unsupported RV64 object lowering|\[rv64-gcc-torture\]|pass\t|fail\t|RV64_C4C_OBJ|error:' build/agent_state/395_step4_divmod_residual_probe.log && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Result: build succeeded; allowlisted probe completed with `total=1 passed=0
failed=1`; `src/divmod-1.c` still fails at
`unsupported_instruction_fragment`; backend CTest passed all selected
`^backend_` tests with `100% tests passed, 0 tests failed out of 326`.
`test_after.log` is the canonical proof log.

Log paths:
- `build/agent_state/395_step4_divmod_residual.allowlist.txt`
- `build/agent_state/395_step4_divmod_residual_probe.log`
- `build/agent_state/395_step4_divmod_residual_prepared.txt`
- `build/agent_state/395_step4_divmod_residual_codegen.err`
- `build/agent_state/395_step4_divmod_residual_i8_call.c`
- `build/agent_state/395_step4_divmod_residual_i8_call.prepared.txt`
- `build/agent_state/395_step4_divmod_residual_i8_call.obj.err`
- `build/agent_state/395_step4_divmod_residual_i16_call.c`
- `build/agent_state/395_step4_divmod_residual_i16_call.prepared.txt`
- `build/agent_state/395_step4_divmod_residual_i16_call.obj.err`
- `build/agent_state/395_step4_divmod_residual_minimize.results.txt`
- `build/rv64_gcc_c_torture_backend/src_divmod-1.c/case.log`
- `test_after.log`
