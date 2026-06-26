Status: Active
Source Idea Path: ideas/open/395_rv64_object_route_instruction_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Iterate Or Split Remaining Families

# Current Packet

## Just Finished

Step 4 - Iterate Or Split Remaining Families added reusable RV64 object
lowering for prepared scalar integer `bir.udiv` in
`fragment_for_prepared_binary()`. The object route now materializes supported
integer literal/home/register operands into scratch GPRs, emits `divu` for i64
and `divuw` for i32, and publishes register or stack-slot results using the
result scalar width.

The same packet also generalized the object-route integer immediate loader
used by prepared operand movement from signed-12-bit-only `addi` materialization
to recursive 12-bit chunk materialization. This is required for the
`src/20000402-1.c` immediate/immediate `bir.udiv i64 85899345920, 2147483648`
representative and for the preceding prepared large-literal local stores; it is
not filename- or constant-specific.

## Suggested Next

Classify the next `unsupported_instruction_fragment` representative after the
now-passing `src/20000402-1.c` subset, then delegate one semantic family only.

## Watchouts

- Do not treat the green `20000402-1.c` proof as closing the broader Step 4
  family list; the next representative still needs fresh classification.
- `20000403-1.c` was previously classified as a call-argument materialization
  gap for immediates outside signed-12-bit range. The new generic object-route
  immediate loader may change its first blocker, so re-probe before delegating.
- Do not mix `20000622-1.c` into the udiv packet; its frame-slot call arguments
  are explicitly marked with missing publication facts.
- Keep aggregate/sret local-frame-address handling from `20000917-1.c` and
  broader large-literal comparison pressure from `20001221-1.c` as separate
  packets unless a later proof shows they collapse into the same producer/target
  fact.

## Proof

Ran the delegated RV64 proof exactly, then recopied the one-case result to
`build/agent_state/395_step4_udiv_proof.log` after supervisor-side regression
guarding. Result: `[rv64-gcc-torture] total=1 passed=1 failed=0`, with
`pass	src/20000402-1.c`.

Delegated command:
`cmake --build --preset default --target c4cll && printf '%s\n' 'src/20000402-1.c' > build/agent_state/395_step4_udiv.allowlist.txt && { ALLOWLIST=build/agent_state/395_step4_udiv.allowlist.txt STOP_ON_FAILURE=0 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log 2>&1 || true; } && rg -n 'unsupported_instruction_fragment|unsupported RV64 object lowering|\[rv64-gcc-torture\]|pass\t|fail\t|RV64_C4C_OBJ' test_after.log`

Supervisor validation:
- `ctest --test-dir build -j --output-on-failure -R '^backend_'` passed
  before and after the slice: 326 passed, 0 failed.
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
  passed.
- `build/agent_state/395_step4_post_udiv_representatives.log` re-ran the five
  classified representatives: `src/20000402-1.c` and `src/20001221-1.c` now
  pass; `src/20000403-1.c`, `src/20000622-1.c`, and `src/20000917-1.c` still
  report `unsupported_instruction_fragment` and remain separate families.
