Status: Active
Source Idea Path: ideas/open/100_aarch64_00204_stdarg_hfa_runtime_repair.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Prove The AArch64 Targeted Cases

# Current Packet

## Just Finished

Traced the Step 5 remaining AArch64 HFA float/double overflow-source family
after the F128 lane-copy repair and left implementation unchanged.

- Verified the representative `%hfa13 %hfa13 %hfa13 %hfa13` caller plan uses
  `arg.source_selection=frame_slot_value` for overflow HFA float lanes:
  the selected caller-side source slots are `%t783.hfa0..2` at stack offsets
  `7340,7344,7348` and `%t789.hfa0..2` at `7364,7368,7372`, while the
  outgoing overflow destinations are `0,4,8` and `16,20,24`.
- Verified the emitted caller assembly copies those selected source slots to
  the outgoing overflow area through `x16 = sp - 32` before the final
  `sub sp, sp, #32`; the codegen is not accidentally copying from the later
  call-argument spill slots `15784+`.
- Verified the `myprintf` variadic entry initializes `overflow_arg_area` to
  incoming SP (`sp + frame_size`) and the HFA float/double `va_arg` overflow
  paths read the same packed lane offsets that the caller writes, advancing by
  the aligned aggregate stride.
- Used Clang `-target aarch64-linux-gnu -S -O0` as an ABI oracle for
  `00204.c`; it also stages overflow HFA lanes contiguously at offsets
  `0,4,8,12,...` for float HFAs, so changing to padded per-lane stack slots
  would be a size/align guess and an overfit risk.
- Found a plausible stack-lifetime route: the current backend writes the
  outgoing area below SP and only adjusts SP in the printed call instruction.
  A safe local repair would require either reserving outgoing call space in the
  prepared frame or coordinating earlier SP adjustment with old-SP rebasing of
  prepared frame-slot sources. A calls.cpp-only attempt would break existing
  SP-relative source loads, and the call-boundary printer currently requires
  prepared frame-slot memory for frame-slot register moves.
- No implementation files, broad expectations, or `00204.c` dump expectations
  were changed.

## Suggested Next

Delegate a plan-owner/reviewer decision on whether to split a new stack-lifetime
packet for AArch64 outgoing call arguments. The next implementation slice should
either reserve outgoing stack argument space as part of the function frame or
explicitly model old-SP frame-slot rebasing before moving SP earlier; it should
not patch only HFA lane offsets or `00204.c` output.

## Watchouts

- Do not downgrade `00204.c` expectations, mark it unsupported, or special-case
  its literal output shape.
- Keep `00032.c` and `00182.c` visible as guard cases.
- `00204.c` still fails the selected proof. The HFA long-double first-lane
  repetition remains repaired, but `00204.c` still has a separate stdarg
  aggregate text mismatch plus HFA float/double overflow corruption.
- The remaining HFA double/float corruption is an overflow-only family: the
  first two HFA aggregates are correct, then values copied from caller-side
  frame slots into the outgoing stack area become zero, NaN, or large garbage.
  Example current output includes `HFA double: 24.1,24.4 24.1,24.4 0.0,0.0`
  and `HFA float: 14.1,14.4 14.1,14.4 -0.0,-107374184.0`.
- The caller and callee agree on packed overflow HFA lane offsets for the
  representative float case, and Clang confirms that layout. Do not repair this
  by padding float lanes to 8 bytes.
- The safe stack-lifetime repair is broader than one HFA testcase: it must
  preserve source frame-slot addressing if SP is adjusted before outgoing
  stack-argument stores.
- Do not repair this by size/align guessing or by weakening expectations; that
  would be an ABI overfit risk for non-HFA scalar or aggregate cases.

## Proof

Ran:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_.*|backend_prepare_frame_stack_call_contract|c_testsuite_aarch64_backend_src_(00204|00032|00182)_c)$' > test_after.log 2>&1
```

Result: exit code 8. The selected build completed successfully, and CTest ran
the contained 26-test subset. 25/26 selected tests passed. The only selected
failure remains `c_testsuite_aarch64_backend_src_00204_c`; guard cases
`c_testsuite_aarch64_backend_src_00032_c` and
`c_testsuite_aarch64_backend_src_00182_c` passed, as did
`backend_prepare_frame_stack_call_contract` and the selected AArch64 route
tests. `00204.c` still fails with remaining stdarg aggregate text mismatch and
HFA float/double overflow-source corruption; the long-double HFA
lane-repetition family remains repaired.

Canonical executor proof log: `test_after.log`.
