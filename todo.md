Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Residual Buckets

# Current Packet

## Just Finished

Step 2 reran the 12 non-timeout AArch64 backend residual representatives from
the accepted baseline. The focused proof still fails all 12, so the Step 2
output is a classification packet, not a repair packet.

Proof result:

- Command:
  `ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_aarch64_backend_src_00112_c|c_testsuite_aarch64_backend_src_00143_c|c_testsuite_aarch64_backend_src_00157_c|c_testsuite_aarch64_backend_src_00163_c|c_testsuite_aarch64_backend_src_00174_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00182_c|c_testsuite_aarch64_backend_src_00183_c|c_testsuite_aarch64_backend_src_00187_c|c_testsuite_aarch64_backend_src_00205_c|c_testsuite_aarch64_backend_src_00216_c|c_testsuite_aarch64_backend_src_00218_c)$' > test_after.log 2>&1`
- Result: 0% tests passed, 12 tests failed out of 12.
- Fresh log: `test_after.log`.

Semantic residual buckets:

- `00143`: backend compile/assembler failure from duplicate generated labels in
  `build/c_testsuite_aarch64_backend/src/00143.c.s`, starting with
  `.Lselect_mat_1_24_164_37_true` and its `_end` label being defined more than
  once. Bucket: synthetic select/materialized label uniqueness, likely AArch64
  backend label allocation for select-matrix lowering.
- `00112`: runtime nonzero on `return "abc" == (void *)0;`. Generated `main`
  returns stale `x13` with `mov x0, x13; ret` instead of materializing the
  string-literal pointer/null comparison result. Bucket: pointer constant
  comparison result publication.
- `00157`: runtime mismatch in indexed local array stores for the square table.
  The first loop computes element addresses/products but generated stores copy
  from uninitialized fixed stack slots into array elements. Bucket: indexed
  local aggregate element store/writeback.
- `00163`: runtime mismatch `bolshevic.b = 42` instead of `34`. After assigning
  `b = &(bolshevic.b)`, generated code later reloads the old `&a` stack value
  before dereferencing. Bucket: local pointer variable reassignment/writeback to
  an addressed aggregate field.
- `00174`: runtime mismatch across scalar floating arithmetic outputs, with
  early float/double arithmetic printing zeros while simple later call results
  still appear. Bucket: scalar FP expression result materialization/publication,
  separate from variadic HFA call-boundary publication.
- `00176`: runtime mismatch after quicksort; the initial array prints correctly
  but the sorted array remains wrong. Bucket: indexed global array
  selected-address writeback in swap/quicksort paths.
- `00182`: runtime mismatch in LED digit rendering; expected digits 1 through 7
  render as zero-shaped digits. Bucket: digit extraction/local array
  publication around unsigned div/rem or local element materialization; current
  visible evidence is output-only, so this should not be claimed as the old
  stale div/rem fault without a fresh bounded artifact.
- `00183`: runtime mismatch for conditional expression output; expected
  `Count*Count` below five and `Count*3` afterward, actual output is raw
  `Count`. Bucket: conditional operator/select value producer materialization.
- `00187`: runtime mismatch where the first `fread` path prints
  `couldn't read fred.txt`, then later file operations print expected lines.
  Bucket: libc call-boundary return/count publication for file I/O.
- `00205`: runtime mismatch with empty output for the global `cases[]` table
  loop. Bucket: constant `sizeof`/global aggregate object metadata or
  initializer layout causing the loop bound to collapse.
- `00216`: runtime nonzero segfault with no stdout in the large aggregate and
  compound initializer test. Bucket: complex aggregate initializer/object layout
  or global relocation semantics; it needs a bounded first-bad artifact before
  becoming the leading owner.
- `00218`: runtime mismatch, unexpectedly printing
  `unsigned enum bit-fields broken`. Bucket: unsigned enum bit-field
  storage/load layout or zero-extension, distinct from MOVI immediate
  publication.

Timeout quarantine:

- `00200` and `00207` remain timeout cases from the accepted baseline and were
  intentionally excluded from this non-timeout proof command.
- They should not influence focused owner selection unless a later bounded run
  captures a concrete first bad fact.

Existing open-idea mapping and rejected adjacent owners:

- `00143` has no exact open owner. It is adjacent to
  `352_aarch64_block_label_emission_ordering.md`, but that parked route was
  about basic-block label/epilogue ordering, while this fresh fact is duplicate
  synthetic select labels inside generated AArch64 assembly. It is also not the
  old `00143` scalar-cast/source-publication thread.
- `00112` is adjacent to string pointer publication work, but the current open
  `356_semantic_bir_pointer_derived_string_loads.md` is about dynamic
  pointer-derived byte loads; this representative is a string-literal
  pointer/null comparison with no byte load.
- `00157` and `00176` map to the indexed aggregate writeback family that has
  appeared in closed/parked history, but no current open idea exactly owns
  local/global indexed element writeback as the next focused route.
- `00163` is adjacent to materialized pointer writeback ideas, including the
  parked `361_aarch64_materialized_pointer_storelocal_writeback.md`, but this
  failure is specifically a local pointer variable reassignment to a global
  field address before dereference.
- `00174` is not `326_aarch64_variadic_hfa_floating_residual.md`; this is
  scalar FP expression lowering, not variadic HFA/floating call-boundary
  publication.
- `00182` is adjacent to `350_aarch64_unsigned_div_rem_producer_publication.md`,
  but the current all-zero digit rendering needs a fresh first-bad artifact
  before reusing that parked div/rem owner.
- `00218` is not `332_aarch64_movi_zero_extension_publication.md`; the failure
  is enum bit-field layout/load publication.

Leading focused owner candidate:

- Lead with `00143`: create or switch to a focused owner for AArch64 synthetic
  select/materialized-label uniqueness. It has the most concrete first bad
  fact in this inventory pass, fails before runtime, and the proof artifact
  names duplicate labels directly in generated assembly.
- Rejected as the immediate lead: `00216` because it still needs a bounded
  crash first bad fact; `00182` because current output does not yet prove the
  stale div/rem owner; `00157`/`00176` because indexed aggregate writeback is a
  broader bucket; `00174` because scalar FP arithmetic has a wider blast
  radius; `00112` because it is small and promising but less directly connected
  to an existing backend label-generation owner than the assembler-diagnostic
  `00143` representative.

## Suggested Next

Execute Step 3 from `plan.md`: decide whether to split a new focused open idea
for `00143` synthetic select/materialized-label uniqueness, or deliberately
choose a different bucket despite the compile-stage evidence above.

## Watchouts

- The `00143` label failure should be kept distinct from testcase-overfit label
  spelling fixes. The owner should be the synthetic label allocation rule, not
  the concrete generated label names in this one file.
- `00200` and `00207` are still quarantined timeouts.
- The non-backend workflow and LLVM GCC torture failures from the full-suite
  baseline stay outside this backend-regex owner selection.

## Proof

Fresh proof command completed and wrote `test_after.log`:

`ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_aarch64_backend_src_00112_c|c_testsuite_aarch64_backend_src_00143_c|c_testsuite_aarch64_backend_src_00157_c|c_testsuite_aarch64_backend_src_00163_c|c_testsuite_aarch64_backend_src_00174_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00182_c|c_testsuite_aarch64_backend_src_00183_c|c_testsuite_aarch64_backend_src_00187_c|c_testsuite_aarch64_backend_src_00205_c|c_testsuite_aarch64_backend_src_00216_c|c_testsuite_aarch64_backend_src_00218_c)$' > test_after.log 2>&1`

Result: 12/12 selected representatives still fail, as expected for the
classification step.
