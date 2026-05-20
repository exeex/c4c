Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Failure Families

# Current Packet

## Just Finished

Step 2: Classify Failure Families. Classified the 53 current
`c_testsuite_aarch64_backend_*` failures from `test_after.log`, generated
assembly under `build/c_testsuite_aarch64_backend/src/`, and representative
source cases. No implementation, test, plan, idea, or review files were
edited.

Semantic owner candidate buckets:

- AArch64 return-result publication / epilogue clobber (22):
  `00004`, `00011`, `00013`, `00014`, `00016`, `00019`, `00020`, `00022`,
  `00052`, `00087`, `00103`, `00112`, `00116`, `00117`, `00118`, `00121`,
  `00124`, `00139`, `00153`, `00159`, `00168`, `00170`.
  Representative evidence: `00011.c.s`, `00022.c.s`, and `00052.c.s` load the
  intended return value and then overwrite `x0` with stale `x13`; `00004.c.s`
  and related pointer-local cases load `w0` from the correct stack slot and then
  overwrite `x0` with restored/stale `x20`; `00159.c.s` computes `myfunc` into
  `w0` and then returns `x13`; `00168.c.s` computes recursive factorial into
  `w0` and then returns `x13`; `00087.c.s` and `00124.c.s` preserve an indirect
  call result and then return a restored callee-saved register.
- Scalar conversion, comparison, and FP/bitfield value lowering (7): `00086`,
  `00111`, `00119`, `00123`, `00144`, `00174`, `00218`.
  Representative evidence: `00086.c.s` reads the uninitialized short slot
  instead of materializing `x = 0`; `00119.c.s` and `00123.c.s` load the global
  double and return register bits without lowering the `< 1` comparison;
  `00174` prints zeros/garbage for double arithmetic and comparisons; `00218`
  prints `unsigned enum bit-fields broken` after the enum bitfield path fails
  to match `AMBIG_CONV`.
- Addressable memory, pointer slot, and indexed aggregate materialization (6):
  `00077`, `00157`, `00171`, `00182`, `00195`, `00208`.
  Representative evidence: `00157.c.s` writes the squared values through a
  copied/unrolled stack-array view and later prints zeros/garbage; `00171.c.s`
  stores `b = &a` at `[sp,#8]` but tests `[sp,#32]`, producing `b is NULL`;
  `00195.c.s` walks every `point_array` slot with static offsets and stores
  from an unrelated FP register, yielding `0.000000, 0.000000`; `00208`
  segfaults on local char-array/struct initialization.
- Control-flow, switch, loop induction, and fallthrough lowering (5): `00158`,
  `00169`, `00200`, `00207`, `00220`.
  Representative evidence: `00158.c.s` compares case `2` against `w21` rather
  than the switch value, so Count 2 takes the default arm; `00169` prints a
  constant garbage middle induction variable; `00200.c.s` emits the test body
  and `0 test(s) failed` but has no visible main epilogue/return at the tail;
  `00207` and `00220` time out, with `00220.c.s` repeatedly re-basing the wide
  string loop at `sp` instead of advancing the stored pointer.
- Call, ABI, varargs/libc, and indirect-call pointer materialization (4):
  `00089`, `00140`, `00186`, `00187`.
  Representative evidence: `00089.c.s` calls the returned function pointer but
  then loads the next function pointer through stale `x19`, causing a bus
  error; `00140` segfaults in the struct-by-value plus variadic-call case;
  `00186` only prints the first `sprintf`/`printf` line before runtime
  mismatch; `00187` times out in repeated stdio read loops.
- AArch64 scalar cast machine-printer operand normalization (9): `00143`,
  `00173`, `00175`, `00176`, `00181`, `00185`, `00204`, `00205`, `00216`.
  Representative evidence: all nine fail before runtime with the same
  AArch64 printer diagnostic: `cannot print AArch64 machine node family=scalar`
  for `sign_extend` or `zero_extend`, because the scalar cast node requires a
  structured register source operand. Generated `.s` files are partial but show
  the surrounding cases are broad scalar/array/call programs, not one named
  testcase shape.

Best next tractable focused owner candidate: AArch64 return-result publication
and epilogue clobber. It owns the largest coherent bucket, has simple current
evidence in very small tests (`00011`, `00022`, `00052`) and call-result tests
(`00159`, `00168`, `00087`, `00124`), and can be probed without touching
expectations or the `stdarg` route. The key acceptance shape should be that the
selected cases no longer emit a correct return/result value followed by a stale
`mov x0, x13`, `mov x0, x20`, `mov x0, x21`, or equivalent restored-register
overwrite.

## Suggested Next

Execute a focused implementation packet for the AArch64 return-result
publication / epilogue-clobber bucket. Start with one no-call scalar return
case and one call-result case so the fix proves both ordinary expression return
publication and post-call result preservation without relying on a single named
fixture.

## Watchouts

The return-result bucket is cross-cutting with memory and call lowering: some
cases may reveal a second bug after the stale final `x0` overwrite is removed.
Do not claim the whole 22-test bucket on one narrow proof unless adjacent
representatives are rerun. The machine-printer bucket is also coherent and
probably tractable, but it is separate from runtime return-result publication.
Leave `review/326_stdarg_byval_route_review.md` untouched.

## Proof

Ran:

`git diff --check`

Result: passed with no whitespace errors. `test_after.log` remains the current
failure-evidence log from Step 1; no test rerun was required for this
classification-only packet.
