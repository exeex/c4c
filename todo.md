Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Current Failure Families

# Current Packet

## Just Finished

Step 2 - Classify Current Failure Families is complete. The current external
AArch64 failures from `test_after.log` classify into these first-bad-fact
buckets:

- `lir_to_bir` admission hole: `00005` fails before assembly with
  `[FRONTEND_FAIL]` and the diagnostic says semantic `lir_to_bir` failed in the
  store local-memory family. The source is pointer-to-pointer local memory
  (`int *p`, `int **pp`, `**pp = 1`), so this is not a runtime codegen owner
  yet.
- Indirect pointer/load result not returned: `00020` and `00103` both compute a
  pointer chain, load through it, then return the address carrier instead of
  the loaded scalar. Representative assembly for `00020` has
  `ldr x9, [x21]` followed by `mov x0, x21`; `00103` has the same shape after
  the `void **` cast. The first bad fact is load-result publication into the
  return register, not pointer arithmetic alone.
- Pointer/FP comparison result not materialized: `00112`, `00119`, and `00123`
  return stale source registers instead of comparison booleans. `00112`
  (`"abc" == (void *)0`) emits only `mov x0, x13; ret`. `00119`/`00123` load
  global double `x` into `d13` and then `mov x0, x13`, returning FP bits rather
  than `x < 1`.
- Static/global aggregate initializers are present but dynamic indexed aggregate
  access is not: `00205`'s `.data cases:` table contains the expected `.xword`
  values, but runtime prints wrong table lanes, so the first bad fact is
  dynamic `cases[j].field[i]` load selection. `00195` similarly stores through
  `point_array+0`, `+8`, `+16`, ... with `str d9` and never forms
  `point_array + my_point * sizeof(struct point)`, so
  `point_array[10].x/y` remain zero.
- Local/multidimensional array address arithmetic: `00130` stores
  `arr[1][3] = 2` at `[sp, #2]` while `p[1][3]` later reads `[base, #7]` and
  `q = &arr[1][3]` is formed as `sp + 1 + 3`. The first bad fact is local array
  row-stride/element-offset calculation for `char arr[2][4]`.
- Recursive call live-value preservation: `00168` prints `1` then corrupt
  factorials. In `factorial`, the recursive path calls `factorial(i - 1)` and
  then multiplies `w20 * w21`, but the incoming `i` is never saved into `w20`
  before the call. The first bad fact is live parameter preservation across a
  recursive direct call.
- Floating constants, FP binops, and FP comparisons: `00174` starts with zeros
  and junk comparison columns. The generated first printf path does
  `fcvt s8, d13` before any materialization of `12.34 + 56.78`, so `d13` is
  stale/uninitialized. This is a focused FP expression-lowering bucket.
- Dynamic indexed global/local mutation and pointer-array recursion:
  `00176`, `00181`, and `00182` all show runtime failure after successful
  assembly in code dominated by indexed arrays and pointer-carried element
  stores. `00176`'s `swap` expands global `array[a]`/`array[b]` into a large
  fixed-lane selection/store sequence; the output remains mostly unsorted.
  `00181` segfaults in Tower of Hanoi after pointer-parameter array moves.
  `00182` prints only partial LED rows from buffer-writing helpers that advance
  `char *p` and store through `*p++`.
- External library call result/stack-buffer publication: `00186` only prints
  `->01<-` even though its loop/count assembly is structurally present; the
  repeated `sprintf(Buf, ...)` / `printf("%s", Buf)` stack buffer path is the
  representative first-bad area. `00187` prints `hch: ...` instead of the first
  `hello\n`, pointing at file I/O buffer/result publication around `fread`
  before the later `fgetc`/`getc` loops.
- AArch64 byval aggregate call lane publication: `00204`'s first mismatch is
  `fa_s1(s1)`: expected `0`, actual is a non-ASCII byte. The callee expects the
  1-byte struct payload in `w0`, but `arg` loads `s1` to a stack temp and calls
  `fa_s1` with `add x0, sp, #928`, so the first bad fact is passing the address
  of the byval temp instead of the packed small-aggregate lane. This lines up
  with the parked byval/HFA/variadic ABI owners, not the generic backend
  inventory.
- Return-value publication after exhaustive macro expansion: `00200` reports
  `0 test(s) failed` but exits `104`. The tail computes `nfailed != 0` into
  `w9`, then overwrites the return with `ldr x0, [sp, #908]`; the first bad fact
  is return register publication, not the left-shift checks themselves.
- Complex aggregate initialization / reloc / flexible-array residual:
  `00216` segfaults with no stdout. The generated `foo` body contains many
  local struct initializers and suspicious early loads from uninitialized stack
  slots such as `[sp, #472]` while building nested aggregates. This is a
  separate aggregate-initializer/compound-literal owner candidate and needs a
  focused probe before implementation.
- Timeout quarantine: `00173` and `00207` timed out at about 5 seconds and were
  not rerun. `00173`'s generated loop repeatedly reloads `.str0` instead of the
  induction pointer and updates `b` to a fixed stack address, consistent with
  an output/infinite-loop pointer-iteration bug. `00207` contains a VLA/goto
  path; its `f1` assembly mutates `sp` around the VLA and loops through label
  control flow. Both should stay quarantined until a bounded single-test probe
  is explicitly delegated.

## Suggested Next

Run Step 3 from `plan.md`: choose a focused owner by comparing the strongest
classified buckets against the existing open parked ideas. The most concrete
owners from this pass are byval aggregate lane publication (`00204`), dynamic
indexed aggregate/global access (`00195`/`00205`, with adjacent `00176`), and
FP expression/comparison lowering (`00119`/`00123`/`00174`).

## Watchouts

- Do not implement fixes under the umbrella inventory plan.
- Do not reopen closed or parked owners from failing counts alone.
- `00005` is still an admission failure, so it should not be grouped with
  runtime codegen regressions until semantic `lir_to_bir` accepts the program.
- `00204` has a precise first mismatch, but it overlaps parked ABI work; route
  selection should compare it with the existing byval/HFA/variadic ideas before
  creating anything new.
- `00173` and `00207` are timeout quarantines from the fresh broad run; avoid
  unbounded reruns or output-storm diagnosis inside the umbrella.

## Proof

No new broad run was required or performed for Step 2. Classification used the
fresh preserved log:
`cmake --build --preset default && ctest --test-dir build -j10 -R backend --output-on-failure | tee test_after.log`

and bounded generated-artifact inspection under
`build/c_testsuite_aarch64_backend/src/*.c.s` for the failing cases. Proof log
remains `test_after.log`.
