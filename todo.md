Status: Active
Source Idea Path: ideas/open/299_aarch64_scalar_immediate_materialize_or_encoding_fallback.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair scalar immediate lowering or fallback

# Current Packet

## Just Finished

Step 2 repaired scalar add/sub/bitwise immediate printing in
`src/backend/mir/aarch64/codegen/alu.cpp`. The machine printer no longer
fail-closes on scalar immediates outside the old plain `#imm` shortcut; it now
keeps directly printable add/sub immediates when valid and materializes
non-direct scalar add/sub/bitwise immediates into an available GP scratch before
emitting the register ALU form. This covers register/memory plus immediate,
immediate plus register/memory, and immediate pair forms while preserving
operand order for `sub`.

Focused printer tests now assert the new capability instead of the old
fail-closed behavior: `add w0, w1, #4096` and `sub w0, w1, #-1` materialize the
RHS into `w9`, and immediate-LHS `sub` materializes the LHS into `x9`.
Logical ALU immediates take the materialization path instead of the old
0..4095 shortcut because values such as `152` are not necessarily valid
AArch64 logical immediates.

The delegated focused CTest subset no longer reports the old diagnostic
`scalar add/sub/bitwise immediate operand is outside the plain #imm encoding
range 0..4095`. `00031` now passes, and `00218` gets past the previous invalid
logical-immediate assembler error and reaches a runtime mismatch.

## Suggested Next

Next packet should address the first remaining focused failure that is outside
the repaired scalar ALU immediate printer. The current earliest hard failures
are symbol-store value printing in `00213`/`00214` and invalid `sxtw w20, w13`
cast spelling in `00104`; supervisor should choose the next owner based on
plan scope.

## Watchouts

- The delegated proof is still red, but the old scalar immediate printer
  diagnostic is gone from `test_after.log`.
- Remaining focused failures after this slice: `00213`/`00214` fail in memory
  symbol-store printing, `00104` fails assembly on cast spelling
  `sxtw w20, w13`, `00207`/`00215` segfault, `00143` times out, and `00218`
  reaches runtime mismatch (`unsigned enum bit-fields broken`).
- Do not treat the remaining runtime/timeout failures as scalar immediate
  materialization failures without fresh evidence from generated code.

## Proof

Ran the delegated proof command exactly after implementation:

`cmake --build --preset default && ctest --test-dir build -j8 -R '^c_testsuite_aarch64_backend_src_(00031|00104|00143|00207|00213|00214|00215|00218)_c$' --output-on-failure > test_after.log 2>&1`

Build completed; focused CTest returned failure status with 1/8 passing and
preserved the proof log at `test_after.log`. Additional local focused check:
`cmake --build --preset default --target backend_aarch64_machine_printer_test
&& ctest --test-dir build -R '^backend_aarch64_machine_printer$'
--output-on-failure` passed.
