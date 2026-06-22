Status: Active
Source Idea Path: ideas/open/306_rv64_c_testsuite_undefined_main_triage_and_followups.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Capture Representative Evidence

# Current Packet

## Just Finished

Step 2, Capture Representative Evidence, generated semantic BIR,
prepared-BIR, and RV64 asm evidence for `src/00024.c`, `src/00025.c`, and
`src/00094.c` under `build/rv64_c_testsuite_probe_v2/evidence/`, then
summarized the mechanism in
`build/rv64_c_testsuite_probe_v2/representative_evidence.md`.

## Suggested Next

Begin the next packet at RV64 asm emission from prepared BIR: explain or repair
why prepared functions with `main` and valid control-flow summaries emit only
`.text` instead of a `main` symbol and body. Keep global/string address
materialization as a follow-up hazard, not the only root cause.

## Watchouts

- Semantic BIR and prepared BIR both contain `main` for all three
  representative cases; do not chase this as a frontend loss of `main`.
- `src/00024.c` proves prepared global-symbol addressing for `v` at offsets `0`
  and `4`, but final RV64 asm still emits only `.text`.
- `src/00025.c` records a missing structured `LinkNameId` for string constant
  `@.str0`; that is a real address-materialization hazard, but not sufficient
  to explain `src/00094.c`.
- `src/00094.c` has no prepared storage/addressing demands and still emits only
  `.text`, so the primary failure is function-body emission from prepared BIR.
- Do not register the full 220-case sweep as required CTest coverage.
- Do not treat empty `.text` output as a valid successful unsupported path.
- Keep root-level scratch logs clean.

## Proof

Ran the delegated proof command:

```sh
{ cmake --build --preset default && mkdir -p build/rv64_c_testsuite_probe_v2/evidence && for n in 00024 00025 00094; do src="tests/c/external/c-testsuite/src/${n}.c"; ./build/c4cll --dump-bir --target riscv64-linux-gnu "$src" > "build/rv64_c_testsuite_probe_v2/evidence/src_${n}_bir.txt"; ./build/c4cll --dump-prepared-bir --target riscv64-linux-gnu "$src" > "build/rv64_c_testsuite_probe_v2/evidence/src_${n}_prepared_bir.txt"; ./build/c4cll --codegen asm --target riscv64-linux-gnu "$src" -o "build/rv64_c_testsuite_probe_v2/evidence/src_${n}_recheck.s"; sed -n '1,12p' "build/rv64_c_testsuite_probe_v2/evidence/src_${n}_recheck.s"; done; } > test_after.log 2>&1
```

Result: passed. The proof log is `test_after.log` and shows the build was
up-to-date followed by three asm heads, each containing only `.text`.
