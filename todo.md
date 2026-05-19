# LIR To BIR Global Pointer Aggregate Projection Todo

Status: Active
Source Idea Path: ideas/open/298_lir_to_bir_global_pointer_aggregate_projection.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Focused Projection Failures

# Current Packet

## Just Finished

Step 1 diagnosis completed for idea 298 focused projection residuals using
`test_before.log` as the failure source of truth. No implementation files,
expectations, lifecycle source files, or canonical logs were modified.

Focused residual inventory:

- `00176` (`swap`): old failure shape is `[FRONTEND_FAIL]` from the
  AArch64 backend runner with semantic `lir_to_bir` failing outside admitted
  buckets; latest function failure is `swap` in `gep local-memory semantic
  family`. Source/IR evidence: global `int array[16]`; LLVM contains
  `getelementptr i32, ptr @array, i64 %...` for dynamic indexes in `swap`,
  `partition`, and `main`. Category: global scalar-array GEP with dynamic
  element index.
- `00181` (`PrintAll`): old failure shape is the same admitted-bucket
  rejection; latest function failure is `PrintAll` in `gep local-memory
  semantic family`. Source/IR evidence: globals `A`, `B`, and `C` are
  `int[4]`; LLVM contains dynamic `getelementptr i32, ptr @A/@B/@C, i64 %...`
  and decay GEPs passed into `Hanoi`. Category: global scalar-array GEP plus
  global array-to-pointer decay.
- `00182` (`print_led`): old failure shape is the same admitted-bucket
  rejection; latest function failure is `print_led` in `gep local-memory
  semantic family`. Source/IR evidence: `print_led` advances caller-provided
  `char *buf` and indexes static local `d[MAX_DIGITS]`; LLVM contains
  pointer-base `getelementptr i8, ptr %...` and
  `getelementptr i32, ptr @__static_local_print_led_3, i64 %...`. Category:
  pointer-value GEP plus static-local array GEP.
- `00209` (`f4`): old failure shape is the same admitted-bucket rejection;
  latest function failure is `f4` in `gep local-memory semantic family`.
  Source/IR evidence: `f4(fptr4 fp, int i)` indexes an array-of-function-
  pointers parameter; LLVM contains `getelementptr ptr, ptr %p.fp, i64 %...`.
  Category: pointer-parameter GEP over pointer-sized elements.
- `00195` (`main`): old failure shape is the same admitted-bucket rejection;
  latest function failure is `main` in `gep local-memory semantic family`.
  Source/IR evidence: global `struct point point_array[100]`; LLVM contains
  dynamic `getelementptr %struct.point, ptr @point_array, i64 %...` followed by
  field GEPs for `.x` and `.y`. Category: global aggregate-array member GEP.
- `00205` (`main`): old failure shape is the same admitted-bucket rejection;
  latest function failure is `main` in `gep local-memory semantic family`.
  Source/IR evidence: global `PT cases[]` with nested `I c[4]`; LLVM contains
  `getelementptr %struct._anon_3, ptr @cases, i64 %...`, field GEPs, and
  nested `getelementptr i64, ptr %..., i64 %...` for `cases[j].c[i]`.
  Category: global dynamic aggregate member GEP with nested array field.
- `00204`: old failure shape is different from the function-level GEP cases:
  `[FRONTEND_FAIL]` reports bootstrap `lir_to_bir` support only covers scalar
  integer/pointer globals, linear integer-array globals, and aggregate-backed
  globals with honest byte-address semantics. Source/IR evidence: many global
  struct/HFA objects such as `s1` through `s17`, `hfa11` through `hfa34`, then
  later function-level aggregate member GEPs. Category: bootstrap/global
  aggregate semantics before prepared-module handoff, not just a single
  function GEP admission.
- `00216` (`foo`): old failure shape is the same admitted-bucket rejection;
  latest function failure is `foo` in `gep local-memory semantic family`.
  Source/IR evidence: `foo(struct W *w, struct pkthdr *phdr_)` projects through
  pointer parameters (`w->t.s`, `((const struct W *)w)->t.t`,
  `phdr->daddr`, `phdr->saddr`) while the file also includes global aggregate
  initializers, `struct W` with flexible-array member, global `gw`, and global
  `phdr`. Category: boundary pointer-parameter aggregate projection with
  flexible-array/global-aggregate context. Judgment: keep it in this owner as
  a Step 4 boundary probe for now, but do not let it drive Step 2; split later
  if global/pointer/aggregate projection admission removes the old failure for
  the simpler targets but `00216` still fails for flexible-array-specific
  semantics.

## Suggested Next

Start Step 2 with a narrow semantic implementation packet for global scalar
array and pointer-derived GEP admission. Suggested first target set:
`00176`, `00181`, `00182`, and `00209`.

Suggested focused proof subset for that packet:

```bash
cmake --build build --target c4cll
ctest --test-dir build -R 'c_testsuite_aarch64_backend_src_(00176|00181|00182|00209)_c$' -j --output-on-failure > test_after.log
```

If that packet moves all four targets past the old admitted-bucket diagnostic,
use a follow-up Step 3 packet for aggregate/bootstrap coverage with
`00195`, `00205`, and `00204`, then keep `00216` as the Step 4 boundary check.

## Watchouts

- `00204` is not currently failing with the same function-level
  `gep local-memory semantic family` diagnostic; it is a bootstrap/global
  aggregate admission failure and should not be hidden behind a GEP-only fix.
- `00182` combines pointer-value increments with static-local global-array
  indexing; a fix that admits only named globals may leave the pointer-base
  part behind.
- `00216` should not be used as the first implementation driver because it
  combines pointer-parameter aggregate projection with flexible-array and
  broad aggregate-initializer coverage.
- Do not absorb machine-printer residuals, runtime nonzero/mismatch buckets,
  or standalone timeout `00220` without new lifecycle evidence.
- Do not touch expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, CTest registration, implementation files outside
  the focused code path, or canonical logs during lifecycle setup.
- Do not reopen idea 297 from failing counts alone.

## Proof

Diagnosis/inventory only. No proof command was delegated, no broad tests were
run, and `test_after.log` was not modified. Evidence inspected:
`test_before.log`, focused C sources, and temporary LLVM dumps under `/tmp`
generated with `./build/c4cll --codegen llvm --target aarch64-linux-gnu`.
