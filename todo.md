Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Select The Next Focused Owner

# Current Packet

## Just Finished

Step 2: Classify Current Failure Families.

Classified the fresh Step 1 backend regex residuals by first bad fact from the
existing `test_after.log`, `build/Testing/Temporary/LastTest.log`, and generated
assembly under `build/c_testsuite_aarch64_backend/src/`. The Step 1 inventory
counts are preserved: CTest selected 354 backend-matching tests; 334 passed, 18
failed non-timeout, and 2 timed out, for 20 total non-passing tests. The local
backend/unit/CLI subset stayed clean at 142/142 passing; all residuals are
`c_testsuite_aarch64_backend_*`.

Failure buckets:

- Semantic admission/local pointer store: `00005.c` is the only frontend
  failure. First bad fact: `semantic lir_to_bir function 'main' failed in store
  local-memory semantic family` while lowering `**pp = 1`; no runtime artifact
  exists for this case.
- Pointer indirection returns address instead of pointee: `00020.c` and
  `00103.c` both return nonzero pointer low bytes for `**pp`/`**(int**)bar`
  even though the pointee is zero. First bad fact in both generated assemblies:
  after loading through the outer pointer, the return path uses `mov x0, x21`
  rather than loading the final integer pointee.
- Boolean/comparison result materialization: `00112.c`, `00119.c`, `00123.c`,
  and `00200.c`. First bad facts: string-null and double-global comparisons do
  not materialize a 0/1 result (`00112.s` returns stale `x13`; `00119.s` and
  `00123.s` load `d13` then move raw bits to `x0` without `fcmp`/boolean
  lowering). `00200.c` prints `0 test(s) failed` but exits 46, so the final
  `return nfailed != 0` is not the value returned after the last call.
- Indexed aggregate addressing/writeback: `00130.c`, `00176.c`, `00182.c`,
  `00187.c`, and `00195.c`. First bad facts: `00130.s` stores `arr[1][3]` at
  `[sp,#2]` instead of the later checked `[sp,#7]`; `00176.c` prints the input
  array unchanged after quicksort except the last element, consistent with
  broken global indexed swap/writeback; `00182.c` loses most LED digits after
  buffer writes; `00187.c` outputs `hch...` because the local buffer terminator
  lands at the wrong byte; `00195.s` walks many `point_array+N` addresses but
  stores `d9` to each, and runtime prints `0.000000, 0.000000` instead of the
  indexed struct fields.
- Recursive call/local state preservation: `00168.c` factorial output becomes
  corrupt from the second result onward. First bad fact: recursive multiply uses
  `mul w0, w20, w21`, but the current argument/local factor is not preserved
  into `w20` before the recursive call.
- Floating-point expression and FP variadic call lowering: `00174.c`. First bad
  fact: float/double arithmetic and comparisons mostly print zero or stack
  garbage; generated assembly repeatedly passes stale FP values to `printf`
  without the expected `fadd`/`fsub`/`fmul`/`fdiv`/`fcmp` result sequence.
- Recursive pointer/array mutation crash: `00181.c` Tower of Hanoi segfaults.
  First bad fact from source plus generated assembly shape: recursive functions
  mutate global tower arrays through pointer parameters, while generated code
  expands fixed global element snapshots and address-selected stores, matching a
  pointer-indexed aggregate writeback owner rather than a generic runtime issue.
- Static aggregate initializer and relocation materialization: `00205.c` and
  `00216.c`. First bad facts: `00205.c` prints the same corrupted long value
  for every `cases[j].c[i]` and a pointer-like `b` value; `00216.c` segfaults in
  a file dominated by nested/compound/designated initializers, flexible array
  initialization, and function-pointer relocs. Generated `00216.s` also reads
  uninitialized stack slots while building local aggregate values.
- Timeout-only, bounded before execution: `00173.c` and `00207.c`. First bad
  facts available without rerun: `00173.c` times out in string/pointer-copy
  loops using `*dest++ = *src++`; `00207.c` times out in goto/VLA and
  short-circuit control-flow coverage. Treat both as bounded reproduction
  candidates before any broad rerun.
- Unresolved classification item needing owner confirmation: `00186.c` prints
  only the first `sprintf`/`printf` loop line (`->01<-`) instead of `->01<-`
  through `->20<-`. First bad fact from the generated assembly is that the loop
  update stores the incremented count through a temporary slot and then reloads
  into the loop variable, but the runtime exits the loop after the first
  formatted print; this may share the loop/update state bug exposed by the
  aggregate-heavy cases, but it should be owner-confirmed before implementation.

All failures remain in the Step 1 non-passing set:
`00005`, `00020`, `00103`, `00112`, `00119`, `00123`, `00130`, `00168`,
`00173` timeout, `00174`, `00176`, `00181`, `00182`, `00186`, `00187`,
`00195`, `00200`, `00205`, `00207` timeout, and `00216`.

## Suggested Next

Step 3 owner selection should choose one semantic owner bucket from this
classification before assigning implementation. The strongest implementation
owners appear to be either indexed aggregate addressing/writeback or
boolean/comparison result materialization; both have multiple representatives
and nearby evidence. Keep `00005.c` separate as an admission repair and keep
timeouts bounded.

## Watchouts

- Do not implement fixes under the umbrella inventory plan.
- Do not reopen closed or parked owners from failing counts alone.
- The delegated command wrote only the CTest portion to `test_after.log`; the
  preceding build completed separately with `ninja: no work to do`.
- The shell pipeline exited successfully because `tee` was last in the pipeline,
  but CTest reported residual backend failures in the captured log.
- Local backend/unit/CLI tests are clean in this inventory; all residuals are
  external `c_testsuite_aarch64_backend_*` cases.
- Some runtime buckets may collapse after the first semantic owner is fixed:
  `00130`, `00176`, `00181`, `00182`, `00187`, and `00195` all show aggregate
  indexing/writeback symptoms and should not be split by testcase shape.
- Keep timeout cases `00173.c` and `00207.c` bounded before trusting broad
  runtime logs. No timeout reproduction was run in this packet.
- `00186.c` is explicitly accounted for as unresolved classification evidence
  above; do not treat it as a separate implementation owner until Step 3
  confirms whether it belongs with loop/update state or aggregate writeback.

## Proof

No rerun by delegation. Classification used the already captured Step 1 proof:

`cmake --build --preset default && ctest --test-dir build -j10 -R backend --output-on-failure | tee test_after.log`

Read-only evidence sources: `test_after.log`,
`build/Testing/Temporary/LastTest.log`, and generated assembly files under
`build/c_testsuite_aarch64_backend/src/*.c.s`. No root-level logs were modified.
