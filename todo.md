Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Residual Buckets

# Current Packet

## Just Finished

Step 2 classified the 12 current external AArch64 backend residuals from
`test_after.log`, generated assembly under
`build/c_testsuite_aarch64_backend/src/`, source files under
`tests/c/external/c-testsuite/src/`, and existing open/closed idea scopes.
No implementation or test files were modified.

Current backend-regex surface remains Step 1's `356` selected, `344` passed,
`12` failed total, with `10` non-timeout failures and `2` timeouts. Local
backend/unit/route/MIR/BIR/CLI/runtime/smoke tests are green; all residuals
are external `c_testsuite_aarch64_backend_*`.

Classified buckets:

- Dynamic indexed local/global aggregate address and writeback:
  `00157`, `00176`.
  `00157` stores `Count * Count` but generated `00157.c.s` snapshots the local
  array through fixed stack slots and reads high uninitialized slots such as
  `[sp,#92]`, `[sp,#100]`, ... instead of storing through `Array[Count-1]`.
  `00176` current `swap` again expands global `array[...]` through large
  select/snapshot chains and stack temporaries, producing an unsorted array.
  This is fresh evidence that resembles closed idea 348, so Step 3 should not
  blindly reopen it by count; it should ask whether to create/reopen a focused
  selected-address/writeback regression owner based on the current assembly.

- Pointer/subobject address publication:
  `00163`.
  `b = &(bolshevic.b)` should publish `bolshevic+4`, but generated assembly
  reloads the earlier `b = &a` home and prints `42`. The `tsar = &bolshevic`
  path also stores a stack address (`add x9, sp, #56`) instead of the global
  object address while later direct member reads hide part of the defect.
  This is adjacent to closed pointer-derived address/lvalue owners, not enough
  by itself to select a closed owner without a focused split.

- Scalar floating constant/expression materialization:
  `00174`.
  The first lines of `00174.c.s` convert from uninitialized `d13`
  (`fcvt s8, d13`; later `fmov d0, d13`) for `float a = 12.34 + 56.78` and
  the following constant FP arithmetic calls, so the arithmetic prints
  `0.000000` while FP comparisons with explicitly materialized constants later
  work. This is not the open variadic HFA owner 326; it is ordinary scalar FP
  constant/ALU result publication.

- Conditional expression / scalar select arm value materialization:
  `00183`.
  The conditional `(Count < 5) ? (Count*Count) : (Count*3)` branches correctly
  on `Count < 5`, but the then/else arms do not materialize either arithmetic
  arm before the join; the `printf` consumes stale `x13`, producing `0..9`.
  This is adjacent to closed select publication ideas 345/363, but the current
  first bad fact is missing arm expression materialization, not stale stack
  reload over an already computed select result.

- Switch/fallthrough selected value materialization:
  `00182`.
  The LED renderer still emits switch cascades where selected/fallthrough
  paths collapse toward common cases, producing seven zero-like digits instead
  of `1234567`. Earlier unsigned div/rem and frame-layout owners are not the
  current first bad facts; the visible defect is in switch/select arm value
  materialization used by `topline`, `midline`, and `botline`.

- External/libc call result publication:
  `00187`.
  After `fread(freddy, 1, 6, f)`, generated assembly compares stale
  `[sp,#96]` with `6` instead of the returned `x0`, so it prints
  `"couldn't read fred.txt"` even though the later buffer contents are correct.
  This is not dynamic buffer writeback in the current artifacts; it is external
  call return/result publication for libc calls.

- Global aggregate `sizeof` / constant loop-bound materialization:
  `00205`.
  The outer and inner loops compare against uninitialized stack homes
  (`ldr x13, [sp,#40]`, `[sp,#32]`, `[sp,#64]`, `[sp,#56]`) where
  `sizeof(cases)/sizeof(cases[0])` and `sizeof(cases->c)/sizeof(cases->c[0])`
  constants should drive the loop. The program produces no output. This is
  constant/global aggregate metadata materialization, not dynamic indexed
  address writeback yet.

- Unsigned enum bit-field storage/load layout:
  `00218`.
  The source stores `AMBIG_CONV` into an unsigned 8-bit enum bit-field inside
  a union, but generated `main` builds the object at odd stack offsets and
  then calls `convert_like_real` with `x21` instead of the local object
  address. The callee masks an 8-bit load and compares with `152`, but it sees
  the wrong storage and prints `unsigned enum bit-fields broken`. Classify as
  bit-field object layout/address publication before treating it as plain
  compare lowering.

- Complex aggregate initializer / relocation / object layout:
  `00216`.
  Runtime exits with a segmentation fault before output. The source is a dense
  aggregate-initializer and function-pointer relocation case with compound
  literals, flexible arrays, designated ranges, and function pointer tables.
  Current Step 2 did not localize a single first bad instruction beyond this
  object-layout/relocation class; it should remain a lower-ranked
  classification target unless Step 3 chooses initializer layout.

- Timeout quarantine: `00200`, `00207`.
  `00200` times out in a massive macro-expanded constant/type test. Generated
  assembly shows repeated do-while-like control scaffolding and many constant
  `check` calls; this needs a separate timeout localization before owner
  selection. `00207` times out in VLA/goto/dynamic-stack code; generated `f1`
  dynamically adjusts `sp`, reaches a return path without restoring it, and
  contains questionable `[x29,#16]` dynamic-stack bookkeeping. This is adjacent
  to closed dynamic-stack lowering idea 280 but now runtime behavior, not the
  old unresolved helper/unsupported diagnostic.

Closed-scope comparison:

- Closed ideas 348, 294, 345/363, 316, 350, and 280 have adjacent historical
  owners, but current selection should be based on the fresh first bad facts
  above, not reopened from failure counts alone.
- Open idea 326 does not own `00174`; current evidence is ordinary scalar FP
  expression materialization, not variadic HFA/floating call-boundary or
  register-save-area behavior.
- Parked/open pointer string-load idea 356 does not own the current `00163`
  or `00187` facts; neither is a dynamic pointer-derived string byte-load
  collapse.

## Suggested Next

Step 3 candidate semantic owner ranking:

1. Dynamic indexed local/global aggregate address/writeback regression:
   strongest multi-test bucket (`00157`, `00176`) with concrete generated
   assembly evidence. Because this overlaps closed idea 348, supervisor should
   route through plan-owner/reviewer to decide whether this is a regression
   reopen or a new focused follow-up.
2. Scalar FP constant/expression materialization (`00174`): single but very
   clear first bad fact, ordinary FP path, not owned by open HFA/variadic idea
   326.
3. Conditional/switch selected arm value materialization (`00183`, `00182`):
   likely shared select/switch materialization family, but needs tighter
   localization to avoid reopening closed select publication owners for the
   wrong boundary.
4. External/libc call return publication (`00187`): crisp first bad fact and
   probably narrow, but currently single-test.
5. Pointer/subobject address publication (`00163`) and global `sizeof` /
   aggregate metadata (`00205`): both clear enough for later focused ideas but
   weaker as next owner due single representatives.
6. Complex initializer/relocation (`00216`), unsigned enum bit-field layout
   (`00218`), and timeout quarantine (`00200`, `00207`): keep parked until a
   dedicated localization packet identifies a sharper first bad boundary.

## Watchouts

This umbrella remains classification-only. Do not implement fixes under idea
295. Do not reopen closed owners from counts alone; require generated-code,
diagnostic, timeout, or proof evidence that contradicts their closure boundary.
Current evidence says the local backend unit/route surface is green, so Step 2
focused on external AArch64 backend residual families. Several current facts
are adjacent to closed owners; Step 3 should make an explicit lifecycle choice
before assigning implementation.

## Proof

No implementation tests were run for Step 2. Classification used the preserved
Step 1 backend-regex proof log plus read-only inspection commands over
`test_after.log`, generated `build/c_testsuite_aarch64_backend/src/*.c.s`
files, source files under `tests/c/external/c-testsuite/src/`, and relevant
open/closed idea notes.

The preserved Step 1 proof command remains:

```sh
cmake --build --preset default && ctest --test-dir build -j10 -R backend --output-on-failure > test_after.log 2>&1
```

Result: `97% tests passed, 12 tests failed out of 356`.
`test_after.log` is the preserved proof/classification log. The command exited
nonzero because the residual external backend failures are still present.
