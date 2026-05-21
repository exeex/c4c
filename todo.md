Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Residual Buckets

# Current Packet

## Just Finished

Step 2 reran the 11 current non-timeout AArch64 backend residual
representatives after idea 365 closure. The focused proof still fails all 11,
so this packet is classification and routing evidence only.

Proof result:

- Command:
  `ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_aarch64_backend_src_00112_c|c_testsuite_aarch64_backend_src_00157_c|c_testsuite_aarch64_backend_src_00163_c|c_testsuite_aarch64_backend_src_00174_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00182_c|c_testsuite_aarch64_backend_src_00183_c|c_testsuite_aarch64_backend_src_00187_c|c_testsuite_aarch64_backend_src_00205_c|c_testsuite_aarch64_backend_src_00216_c|c_testsuite_aarch64_backend_src_00218_c)$' > test_after.log 2>&1`
- Result: 0% tests passed, 11 tests failed out of 11.
- Fresh log: `test_after.log`.

Semantic residual buckets:

- `00112`: runtime nonzero on `return "abc" == (void *)0;`. Generated
  `main` is just `mov x0, x13; ret` with `.str0` emitted afterward, so the
  string-literal pointer/null comparison is not materialized and a stale
  register becomes the process exit code. Bucket: string-literal pointer
  constant comparison/result publication.
- `00157`: runtime mismatch in local `int Array[10]` square stores. The
  expected `1,4,...,100` output becomes uninitialized-looking values, matching
  the previously observed indexed local array element writeback/store
  materialization bucket.
- `00163`: runtime mismatch on the final `bolshevic.b` print. After the source
  assigns a local pointer to `&bolshevic.b`, generated code reloads the old
  local pointer home at `[sp,#8]` and prints `a == 42`. Bucket: local pointer
  variable reassignment/writeback to an addressed aggregate field.
- `00174`: runtime mismatch across scalar float/double arithmetic, where
  arithmetic expression outputs are zero while comparison lines and later
  library-call-shaped outputs still appear. Bucket: scalar FP expression result
  materialization/publication.
- `00176`: runtime mismatch after quicksort; the initial global array prints
  correctly but the sorted output is wrong. Bucket: indexed global array
  selected-address writeback in swap/quicksort paths.
- `00182`: runtime mismatch in LED rendering; digits 1 through 7 render as
  all zero-shaped digits. Bucket: digit extraction/static digit-array
  publication around unsigned division/remainder or local/global element
  materialization. Fresh output alone is not enough to reactivate the parked
  unsigned div/rem owner.
- `00183`: runtime mismatch for the conditional expression
  `(Count < 5) ? Count*Count : Count*3`; actual output is raw `Count`.
  Bucket: conditional/select expression producer materialization.
- `00187`: runtime mismatch where the first `fread` count check prints
  `couldn't read fred.txt`, while later file reads produce expected content.
  Bucket: libc/file API return-count publication or first `fread` comparison
  handoff.
- `00205`: runtime mismatch with empty output for the global `cases[]` loop.
  Generated `main` compares `j` against values loaded from uninitialized
  stack slots `[sp,#40]` and `[sp,#32]` instead of a constant
  `sizeof(cases)/sizeof(cases[0])` bound. Bucket: global array `sizeof`/loop
  bound constant materialization, adjacent to global aggregate metadata.
- `00216`: runtime nonzero segfault in the large aggregate/compound initializer
  representative. Bucket: complex aggregate initializer/object layout,
  compound literal, or relocation/function-pointer initialization. It needs a
  bounded first-bad artifact before owner selection.
- `00218`: runtime mismatch printing `unsigned enum bit-fields broken`.
  Bucket: unsigned enum 8-bit bit-field storage/load layout or zero-extension,
  distinct from generic MOVI immediate materialization.

Timeout quarantine:

- `00200` and `00207` remain timeout residuals in `test_baseline.log` and
  `test_before.log`; both were intentionally excluded from the non-timeout
  proof command.
- They should not drive owner selection without a later bounded reproducer
  that distinguishes hang, output storm, or slow runtime.

Existing open-idea mapping and rejected adjacent owners:

- `00112` has no exact current open owner. It is adjacent to the string pointer
  publication note in parked
  `356_semantic_bir_pointer_derived_string_loads.md`, but `356` owns dynamic
  pointer-derived byte loads, not a direct string-literal/null pointer
  comparison with no byte load.
- `00157` and `00176` map to the broader indexed aggregate writeback family
  seen in prior inventory history, but no current open idea exactly owns these
  local/global indexed element writeback facts.
- `00163` is adjacent to parked
  `361_aarch64_materialized_pointer_storelocal_writeback.md`, but the fresh
  fact is a local pointer variable reassignment to a global field address
  before dereference, not the parked materialized pointer storelocal shape.
- `00174` is not
  `326_aarch64_variadic_hfa_floating_residual.md`; this representative is
  scalar FP expression lowering, not variadic HFA/floating call-boundary
  publication.
- `00182` is adjacent to parked
  `350_aarch64_unsigned_div_rem_producer_publication.md`, but the current
  all-zero LED output does not prove the stale unsigned div/rem producer
  returned. Reuse that owner only after a fresh first-bad artifact.
- `00218` is not
  `332_aarch64_movi_zero_extension_materialization.md`; the visible failure is
  enum bit-field layout/load behavior.
- Ideas 364 and 365 should stay out of this selection: `00143` now passes and
  is not in the 11-residual proof.

Leading focused owner candidate:

- Lead with `00112`: create or select a focused owner for string-literal
  pointer constant comparison/result publication. It is the smallest current
  first bad fact, has a direct generated-assembly proof (`mov x0, x13; ret`),
  and is not owned by any active open idea.
- Rejected as immediate lead: `00216` because the segfault still needs a
  bounded first-bad artifact; `00182` because the output does not yet prove the
  parked unsigned div/rem owner; `00157`/`00176` because indexed aggregate
  writeback is broader; `00174` because scalar FP arithmetic has wider blast
  radius; `00205` because it is promising but tied to global aggregate/sizeof
  metadata and has more surrounding surface than the one-expression `00112`.

## Suggested Next

Execute Step 3 from `plan.md`: split or select one focused owner for the
`00112` string-literal pointer comparison/result publication residual, unless
the supervisor deliberately chooses a different bucket.

## Watchouts

- Keep `00112` focused on the semantic pointer/result publication rule. Do not
  patch the named source file or special-case `.str0`.
- `00200` and `00207` stay quarantined until bounded timeout evidence exists.
- Do not reopen ideas 364 or 365: `00143` passes after their closure path.
- Do not fold unrelated full-suite failures into this backend inventory without
  explicit supervisor routing; `string_authority_guard` and the 15
  `llvm_gcc_c_torture_*` failures are outside the current AArch64 backend
  residual list.

## Proof

Fresh proof command completed and wrote `test_after.log`:

```sh
ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_aarch64_backend_src_00112_c|c_testsuite_aarch64_backend_src_00157_c|c_testsuite_aarch64_backend_src_00163_c|c_testsuite_aarch64_backend_src_00174_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00182_c|c_testsuite_aarch64_backend_src_00183_c|c_testsuite_aarch64_backend_src_00187_c|c_testsuite_aarch64_backend_src_00205_c|c_testsuite_aarch64_backend_src_00216_c|c_testsuite_aarch64_backend_src_00218_c)$' > test_after.log 2>&1
```

Result: 11/11 selected representatives still fail, as expected for the
classification step.
