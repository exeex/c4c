# RV64 C-Testsuite Undefined-Main Triage And Followups

## Goal

Turn the current RV64 c-testsuite `undefined reference to main` bucket into a
clear, actionable map of backend issues and follow-up repair ideas.

This is a triage/planning idea, not a broad feature implementation. It should
explain exactly why the 93 undefined-main cases happen, distinguish fail-closed
contract bugs from real unsupported feature gaps, and write the next repair
ideas under `ideas/open/`.

## Why This Exists

The first ad-hoc RV64 c-testsuite qemu probe over the current allowlist found:

```text
total=220
pass=62
emit_fail=3
fallback=0
clang_fail=95
qemu_nonzero=60
qemu_timeout=0
mismatch=0
missing=0
```

The largest clang-fail bucket is misleading:

- 93 cases fail at link time with `undefined reference to main`;
- sample emitted assembly for those cases is only:
  ```asm
      .text
  ```
- prepared BIR still contains `main` for representative cases such as
  `src/00024.c`, `src/00025.c`, and `src/00094.c`;
- the likely immediate cause is that `emit_prepared_module_text(...)` returns
  empty text when `append_prepared_global_storage_asm(...)` rejects any global:
  ```cpp
  if (!append_prepared_global_storage_asm(out, module)) {
    return "    .text\n";
  }
  ```

Representative observations:

- `src/00024.c` has a zero-initialized global struct `s v`; prepared BIR has
  `main` with global stores/loads, but RV64 global storage only accepts simple
  `i32`/`i32` array words.
- `src/00025.c` has a string literal plus `strlen`; prepared addressing
  includes `kind=string_constant`, but RV64 string storage/address
  materialization is unsupported.
- `src/00094.c` has `extern int x; int main(){ return 0; }`; this unused extern
  global should not prevent `main` emission, so it exposes an over-strict
  global-storage gate.

This bucket currently mixes several different things:

- fail-closed/output-contract bugs;
- unsupported global storage features;
- unsupported string literal/materialization features;
- unsupported aggregate/global memory features;
- unrelated cases that only appear as linker failures because RV64 emitted an
  invalid successful asm file.

Before adding c-testsuite backend tests as formal CTest coverage, this bucket
needs a clean classification.

## Inputs

Use the existing probe artifacts if present:

- `build/rv64_c_testsuite_probe_v2/summary.tsv`
- `build/rv64_c_testsuite_probe_v2/asm/`
- `build/rv64_c_testsuite_probe_v2/*.emit.out`
- `build/rv64_c_testsuite_probe_v2/*.clang.out`
- `build/rv64_c_testsuite_probe_v2/*.qemu.out`

If they are missing or stale, rerun the same style of non-blocking probe:

```sh
cmake --build --preset default
./build/c4cll --codegen asm --target riscv64-linux-gnu <case> -o <case>.s
/usr/bin/clang --target=riscv64-linux-gnu --gcc-toolchain=/usr -x assembler <case>.s -o <case>.bin -lm
/usr/bin/qemu-riscv64 -L /usr/riscv64-linux-gnu <case>.bin
```

The probe must remain non-blocking and must not register all 220 cases as
required CTest pass/fail tests under this idea.

## In Scope

- Reproduce or validate the 93-case undefined-main bucket from the RV64
  c-testsuite probe.
- Confirm whether each case has prepared BIR with `main`, or whether there is a
  different frontend/semantic handoff issue.
- Classify the 93 cases into root-cause buckets such as:
  - unused extern/global declaration should be ignored by storage emission;
  - string literal storage or string-constant address materialization;
  - aggregate/struct global storage;
  - global aggregate load/store lowering;
  - floating global/scalar storage;
  - pointer-valued global data;
  - other unsupported module/function shape.
- Identify which failures are fail-closed/output-contract defects versus real
  missing RV64 backend features.
- Write follow-up ideas under `ideas/open/` for the next repair slices. Prefer
  a small number of focused ideas rather than one giant c-testsuite-fix idea.
- Include enough case lists or generated summary artifacts under a non-root
  location such as `build/rv64_c_testsuite_probe_v2/` or a durable note inside
  the new follow-up idea text so future agents do not need to repeat the
  detective work.

## Out Of Scope

- Implementing string literal support, aggregate global support, FP global
  support, pointer-valued global initializers, or broad c-testsuite support in
  this triage idea.
- Registering the full c-testsuite RV64 backend sweep as required CTest
  coverage.
- Treating linker `undefined reference to main` as the real root cause without
  inspecting RV64 emitted asm and prepared BIR.
- Weakening existing backend tests or accepting empty `.text` as a valid
  unsupported result.
- Fixing the known `backend_riscv_prepared_edge_publication` baseline failure
  unless a separate idea owns that repair.

## Suggested Execution Order

1. Normalize the probe result:
   - verify total/pass/failure counts;
   - confirm there are exactly 93 undefined-main cases, or document any changed
     count with the command/date used.
2. For representative cases in each rough source category, capture:
   - source snippet;
   - emitted asm head;
   - prepared BIR global/function summary;
   - reason RV64 storage/function emission skipped `main`.
3. Classify all 93 cases into root-cause buckets. Do not rely only on source
   string heuristics when prepared BIR gives a clearer answer.
4. Decide the repair ordering:
   - first, contract cleanup for empty `.text` / fail-closed reporting;
   - then feature buckets ordered by pass-count impact and implementation risk.
5. Create follow-up ideas for the chosen repair slices. Expected candidates
   include, but are not limited to:
   - RV64 module/global storage fail-closed contract cleanup;
   - unused extern global skip/ignore support;
   - RV64 string literal storage and address-materialization foundation;
   - RV64 aggregate/struct global storage foundation;
   - RV64 floating scalar/global support if the bucket is large enough.
6. Close this triage idea only after the follow-up ideas exist and the
   classification is clear enough for agents to start implementation without
   redoing the whole sweep.

## Acceptance Criteria

- The 93 undefined-main cases are listed or summarized with reproducible
  bucket counts.
- At least one representative prepared-BIR/asm explanation is documented for
  each non-trivial bucket.
- The triage explicitly separates:
  - backend output-contract bugs;
  - unsupported but skippable declarations such as unused extern globals;
  - real missing RV64 feature families.
- At least two focused follow-up ideas are created under `ideas/open/`, and
  each has concrete candidate cases plus acceptance criteria.
- No implementation feature is snuck into this triage unless it is tiny
  instrumentation/probe cleanup needed to classify the failures.
- Worktree state remains clean or contains only committed lifecycle/idea
  artifacts when the triage closes.

## Closure

Closed on 2026-06-22 after the triage artifacts under
`build/rv64_c_testsuite_probe_v2/` recorded the 93-case undefined-main bucket,
representative prepared-BIR/asm evidence, per-case classification, and repair
ordering. Follow-up repair work was split into focused open ideas for
`rv64_text_only_fail_closed`, `string_literals_and_extern_calls`, and
`aggregate_global_storage`; smaller pointer, floating, scalar, and
control-shape buckets remain deliberately deferred by the repair-order
artifact rather than folded into this triage plan.

## Reviewer Reject Signals

- The result says "undefined main" without explaining why RV64 emitted only
  `.text`.
- The classification is based only on filenames or rough source regexes when
  prepared BIR evidence is needed.
- The follow-up plan becomes a single giant "make c-testsuite pass" idea.
- The triage registers 220 failing cases as required CTest coverage and breaks
  normal validation.
- Empty `.text` or partial assembly is accepted as a successful unsupported
  path.

## Notes For The Agent

- This idea exists because the current probe result is useful but noisy. Make
  it actionable before chasing individual c-testsuite failures.
- Keep root-level logs clean. Store probe artifacts under `build/` or another
  non-root scratch location.
- Prefer exact case lists in generated artifacts and concise bucket summaries
  in durable idea text.
