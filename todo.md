Status: Active
Source Idea Path: ideas/open/381_aarch64_shift_promotion_codegen_scalability_timeout.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reconfirm Stage and First Slow Phase

# Current Packet

## Just Finished

Step 1 classification/localization complete for idea 381.

- Reconfirmed `c_testsuite_aarch64_backend_src_00200_c` still times out under
  the delegated AArch64 backend CTest proof.
- Direct bounded probes show semantic BIR, prepared BIR, and MIR trace/dump
  complete quickly, while default prepared-stage AArch64 asm generation times
  out.
- First slow backend phase: prepared BIR to AArch64 native asm emission, before
  the output `.s` file is written.
- First localized operation family: prepared call-boundary lowering for the
  expanded `check(...)` call sequence. A bounded gdb sample of the live timeout
  process stopped in
  `c4c::backend::aarch64::codegen::lower_before_call_moves`, called from
  `dispatch_prepared_block` -> `lower_prepared_functions` ->
  `compile_prepared_module`.

## Suggested Next

Execute `plan.md` Step 2 by adding focused scalability coverage for the
prepared AArch64 call-boundary lowering shape: many straight-line `check(...)`
calls with promoted scalar compare/argument values, without keying on `00200`
or `lshift-type.c`.

## Watchouts

- The timeout is not in semantic BIR generation, prepared BIR generation, or
  the current MIR debug surface: `--dump-bir`, `--dump-prepared-bir`,
  `--dump-mir`, and `--trace-mir` all finish in about 0.05-0.11 seconds.
- `--codegen asm --backend-bir-stage semantic --target aarch64-linux-gnu`
  finishes in about 0.05 seconds and writes 44,543 bytes of assembly; the
  default prepared asm route times out at 15 seconds with no output file
  materialized.
- Prepared BIR for `main` reports 626 allocation constraints, 8,566
  interference edges, 223 register assignments, 403 stack slots, 130 calls,
  and 361 blocks. The dominant source shape is repeated `bir.call void check`
  sites after shift/type-promotion folding, not runtime looping.
- Do not work on parked idea 382 unless the supervisor switches lifecycle
  state. Reject testcase-specific shortcuts, timeout policy changes, runner
  changes, CTest registration changes, unsupported-list changes, expectation
  weakening, proof-log policy changes, and count-only progress claims.

## Proof

Delegated proof preserved in `test_after.log`:

`{ cmake --build --preset default && ctest --test-dir build -j1 --output-on-failure -R '^c_testsuite_aarch64_backend_src_00200_c$' --timeout 15 ; } > test_after.log 2>&1`

Outcome: build was up to date; the single CTest timed out after about 5 seconds
per test properties, confirming the backend test still times out.

Bounded diagnostics, with stale-process checks before and after:

- `timeout 15s ./build/c4cll --dump-bir --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00200.c`
  exited 0 in about 0.08 seconds.
- `timeout 15s ./build/c4cll --dump-prepared-bir --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00200.c`
  exited 0 in about 0.10 seconds.
- `timeout 15s ./build/c4cll --dump-mir --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00200.c`
  exited 0 in about 0.09 seconds and identified the current MIR dump as the
  x86/debug summary surface.
- `timeout 15s ./build/c4cll --trace-mir --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00200.c`
  exited 0 in about 0.11 seconds and reported `main` as 361 blocks.
- `timeout 15s ./build/c4cll --codegen asm --backend-bir-stage semantic --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00200.c -o /tmp/c4c_00200_semantic_stage.s`
  exited 0 in about 0.05 seconds.
- `timeout 15s ./build/c4cll --codegen asm --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00200.c -o /tmp/c4c_00200_aarch64.s`
  exited 124 after 15 seconds.
- Bounded stack sample of the default asm timeout process stopped in
  `lower_before_call_moves`, establishing prepared call-boundary lowering as
  the first localized operation family.
