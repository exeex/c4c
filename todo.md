Status: Active
Source Idea Path: ideas/open/628_fpr_abi_frame_policy_and_placement.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh FPR ABI/Frame Evidence

# Current Packet

## Just Finished

Step 1 refreshed FPR ABI/frame evidence for idea 628. Fresh probes are under `/tmp/c4c628_step1_evidence` and used:
`./build/c4cll -I tests/c/external/gcc_torture --dump-bir --target riscv64-linux-gnu <row>`,
`--dump-prepared-bir`, and `--codegen obj`.

Row classification:

- `tests/c/external/gcc_torture/src/980605-1.c`: prepared dump publishes FPR frame authority for `f2`: `saved fpr:fs1`, `slot_placement=slot#57+stack72`, `slot_size=8`, `slot_align=8`, and repeated call-boundary preservation of `%p.x` from `fa0` to `fs1`. RV64 object route rejects the second `getval` call in `f2` with `unsupported_call_abi` despite GPR result shape because the call boundary needs FPR callee-saved preservation support. Bucket: in-scope FPR frame save-slot/preservation consumer.
- `tests/c/external/gcc_torture/src/ieee/compare-fp-2.c`: semantic BIR has `foo(double,double)->i32`; prepared call plan for `main` publishes immediate FPR args to `fa0`/`fa1`, and `foo` publishes saved `fpr:fs1` placement at `slot#2+stack16`. RV64 object route rejects `main` call to `foo` with `unsupported_call_abi`. Bucket: in-scope FPR call-argument consumer, specifically floating immediate/literal materialization into FPR ABI registers; result is GPR and not the blocker.
- `tests/c/external/gcc_torture/src/ieee/unsafe-fp-assoc.c`: prepared call plan publishes immediate FPR arg `0.0` to `fa0` and FPR result `fa0` to `ft0`; `foo` publishes saved `fs1` and `fs2` placements at `slot#1+stack0` and `slot#2+stack8`. RV64 object route rejects `main` call to `foo` with `unsupported_call_abi`. Bucket: in-scope FPR call-argument/result consumer; current first blocker is FPR immediate argument materialization before the already-published FPR result can be consumed. Floating multiply/global `C` semantics stay out of this Step 1 owner bucket.
- `tests/c/external/gcc_torture/src/pr39501.c`: prepared call plans publish many FPR same-module calls, including immediate/register FPR args to `fa0`/`fa1` and FPR results from `fa0` to `ft0` or `fs1`; helper and `main` frame plans publish saved `fs1`/`fs2` placements. RV64 object route rejects the first `main` call to `float_min1` with `unsupported_call_abi`. Bucket: in-scope FPR call-argument/result consumer, with immediate FPR args as the first visible unsupported shape. Floating comparison/min/max semantics are a later/out-of-scope semantic bucket unless still exposed after ABI transport works.
- Nearby FPR-heavy stack-frame row `tests/c/external/gcc_torture/src/ieee/unsafe-fp-assoc-1.c`: prepared dump publishes `func` saved `fs1`/`fs2` placements at `slot#19+stack88` and `slot#20+stack96`, plus a same-module FPR immediate arg to `func`. Current RV64 object route rejects earlier with `prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination` for a non-parallel register-source fan-in to one stack destination. Bucket: out-of-scope move-bundle authority/local stack fan-in, not the next FPR ABI/frame packet.

Shared prepared facts are present for the in-scope rows: FPR register banks are explicit, FPR ABI registers use `fa*`, FPR caller/callee-saved homes use `ft*`/`fs*`, FPR result plans carry `value_bank=fpr`, source/destination placements, and FPR saved-register frame rows carry complete `slot_placement`. The current shared RV64 authority blocker is consumer admission, not missing prepared publication.

## Suggested Next

Execute Step 2: trace exact FPR consumer/producer authority for the smallest code-changing packet. Recommended owner bucket is a narrow RV64 FPR call-boundary consumer packet that validates prepared FPR facts and then admits:

- FPR callee-saved preservation population/republication using explicit `PreparedSavedRegisterSlotPlacement` for `fs*` rows.
- FPR immediate/literal call-argument materialization into prepared `fpr:call_argument` ABI placements.
- Existing prepared FPR register-source call arguments and FPR register results should remain fail-closed and fact-driven.

## Watchouts

Do not collapse this into scalar GPR call lowering with `fa` spellings. Keep the `unsafe-fp-assoc-1.c` fan-in row out of this packet unless the supervisor deliberately opens a move-bundle producer initiative. Keep floating comparisons/min/max semantics, local/global memory repair, runtime mismatch triage, variadic/library policy, unsupported markers, allowlists, timeouts, and `f128` outside idea 628.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.

Result: passed, 347/347 backend tests. Proof log: `test_after.log`.
