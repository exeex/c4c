Status: Active
Source Idea Path: ideas/open/648_rv64_call_arg_frame_slot_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Run Broader Validation And Close Or Park

# Current Packet

## Just Finished

Completed Step 5 broader validation recording for idea 648.

The Step 4 focused and representative proof remains accepted. Focused dump,
text-route, and object CLI coverage for
`riscv64_call_arg_local_frame_address_materialization` passed. Representative
RV64 object emission for `tests/c/external/gcc_torture/src/20000722-1.c`
succeeded, and fresh objdump showed the `bar` call setup materializing the
selected local frame-slot address directly in `a0`:

`98: 00010513      mv a0, sp`

This removes the prior representative stale call-argument shape `mv s2, sp`
followed by `mv a0, s2` before the `foo` call. The later `foo` return-path
`mv a0, s1` remains classified as not the idea 648 `bar` call-argument owner.

The attempted raw broader command
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "riscv64"`
had known unrelated failures, so the supervisor generated a matched
before/after regression pair instead: before at `b63e601f5`, after at current
`main`.

Regression guard command:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`

Result: passed. Counts: before passed=88 failed=18 total=106; after passed=89
failed=18 total=107; delta passed=+1 failed=0; new failing tests=0.

Based on the accepted focused proof, representative integration proof, and
matched non-regressing broader RV64 guard, idea 648 acceptance criteria appear
satisfied.

## Suggested Next

Request plan-owner lifecycle close for idea 648.

## Watchouts

- Use the fresh object disassembly from
  `build/agent_state/648_step4_representative_integration/20000722-1.objdump.txt`
  for representative call evidence.
- The later `foo` return-path `mv a0, s1` is not the `bar` call-argument setup.
  Do not treat that line as a renewed idea 648 failure without a fresh owner
  classification.
- Preserve the `prepared_frame_slot_address_call_argument_offset(...)`
  fail-closed checks; do not bypass them with source-register or stack-offset
  assumptions.
- Do not reopen idea 656 local-memory policy or string-label pointer admission.
- Do not infer frame-slot address materialization from source spelling, stack
  offsets, final assembly, testcase identity, or diagnostic text.
- Do not special-case `src/20000722-1.c`, `%lv._clit_`, `foo`, `bar`, `s1`,
  `s2`, or `a0`.
- Do not edit expectations, unsupported markers, allowlists, timeouts,
  runtime-comparison policy, or pass/fail accounting.

## Proof

No new proof command was run in this recording-only packet. Used the existing
canonical logs and supervisor regression-guard result.

Raw broader command had known unrelated failures:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "riscv64"`

Matched regression guard passed:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`

Canonical logs: `test_before.log`, `test_after.log`.
