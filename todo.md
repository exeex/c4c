Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Residual Failures

# Current Packet

## Just Finished

Step 2 classified the post-337 residuals from `test_after.log` and
representative generated artifacts under
`build/c_testsuite_aarch64_backend/src/`.

Inventory from Step 1 remains:

- Selected: 354 tests
- Passed: 325 tests
- Failed: 26 tests
- Timed out: 3 tests
- Skipped: 0 tests
- Local/internal backend split: 142 selected, 142 passed, 0 failed, 0 timed
  out, 0 skipped
- External `c_testsuite_aarch64_backend_*` split: 212 selected, 183 passed,
  26 failed, 3 timed out, 0 skipped

First-bad-stage buckets for the 29 external residuals:

- Frontend/prepared-node or machine-printer diagnostics: 9 cases.
  - `00143`, `00173`, `00175`, `00176`, `00181`, `00185`, `00204`,
    `00205`: `[FRONTEND_FAIL]` at the AArch64 machine printer with
    `opcode=sign_extend` and `scalar cast node requires a structured register
    source operand`.
  - `00216`: same printer stage, but `opcode=zero_extend`.
  - Semantic owner candidate: AArch64 scalar cast machine-node operand fact
    publication/printing for register-sourced sign/zero extension. This is
    adjacent to, but not the same as, closed idea 334's scalar `mul`/`add`
    operand-fact owner.
- Semantic `lir_to_bir` admission or prepared-module handoff diagnostics:
  0 cases with current evidence. The residuals either reach machine printing
  or runtime.
- Assembler/linker legality: 0 cases with current evidence.
- Runtime nonzero/crash: 8 cases.
  - `00086`: `exit=1`; generated assembly reads `short` from `[sp]`, computes
    `x + 1`, but never stores the initialized/incremented local before the
    comparison.
  - `00111`: `exit=1`; generated assembly stores the `long` local at `[sp]`,
    then reads the `short` local from the callee-save slot `[sp,#8]` and
    returns that stale value after the compound assignment.
  - `00112`: `exit=160`; pointer/null equality lowering does not publish a
    boolean result, and `main` returns `mov x0, x13`.
  - `00119`, `00123`: `exit=160` and `exit=96`; floating comparison lowering
    loads the global double and returns raw/stale `x13` instead of a boolean.
  - `00140`: `exit=Segmentation fault`; aggregate/byval call setup passes
    `x20`/derived pointers without publishing the local struct address or
    byval copy before `f1` dereferences the pointer argument.
  - `00144`: `exit=16`; conditional pointer/null/local pointer publication is
    incoherent, with generated code returning a restored pointer home rather
    than a stable null `q` result.
  - `00200`: `exit=215` while printing `0 test(s) failed`; the final
    `nfailed != 0` value is computed, then the function returns a load from
    `[sp,#908]` instead of the boolean result.
- Runtime mismatch: 9 cases.
  - `00157`: indexed local array store/load lowering; the square value is
    computed but the generated code copies unrelated stack slots instead of
    storing `Array[Count-1]`.
  - `00159`: function formal argument publication; `myfunc` computes
    `mul w0, w20, w20` without first publishing the incoming `w0` argument to
    `w20`.
  - `00170`: variadic call stack-argument publication; the eighth `printf`
    enum argument `h` is not placed in the outgoing stack area, so the first
    output line ends with `0` instead of `75`.
  - `00171`: addressable local pointer publication; `b = &a` is stored at
    `[sp,#8]`, but the null comparison reads `[sp,#32]`, so `b` is treated as
    null.
  - `00174`: floating scalar and floating vararg lowering; generated output is
    mostly zero/garbage across arithmetic, comparisons, and `printf` calls.
  - `00182`: byte-buffer/indexed write construction for the LED display; the
    runtime prints blank rows where switch-selected digit glyph bytes should
    be written.
  - `00186`: stack-frame local layout overlap; `Buf` starts at `sp` while the
    loop counter lives at `[sp,#4]`, so `sprintf` clobbers `Count` and only the
    first line is printed.
  - `00195`: global aggregate indexed double store/load; generated assembly
    walks constant `point_array` offsets and stores stale FP registers instead
    of publishing `point_array[my_point].x/y`.
  - `00218`: unsigned enum bit-field load/store; assignment to the 8-bit
    positive enum field fails to publish the value seen by
    `convert_like_real`, so the default diagnostic prints.
- Timeout or output-storm: 3 cases.
  - `00187`: file I/O loop; generated EOF checks compare the wrong value
    after `fgetc`/`getc`, making the loop unsafe and output-storm prone.
  - `00207`: VLA/goto/control-flow route; generated `f1` mutates/restores `sp`
    around labels and re-enters the label path, so the runtime timeout should
    stay parked until a focused control-flow owner is selected.
  - `00220`: wide-string pointer iteration; generated loop always reloads
    from `[sp]` while storing `p + 4` separately, so the terminator is never
    reached.

Closed-owner boundaries through idea 337:

- Idea 334 remains closed: the current printer diagnostics are scalar
  `sign_extend`/`zero_extend` cast nodes needing structured register operands,
  not the closed scalar `mul` RHS-fact or scalar `add` frame-slot printer
  diagnostics.
- Idea 335 remains closed: no current case replays the closed `00164`
  uninitialized local-slot load evidence. Several runtime cases look like
  storage/publication failures, but their first bad facts are different and
  need focused localization.
- Idea 336 remains closed from counts alone: current `00159` fails because a
  callee formal argument is read from `w20` instead of incoming `w0`, `00170`
  fails at outgoing variadic stack-argument publication, and `00200` shows a
  late non-result stack load. None of those is the closed stale final
  callee-saved return-register overwrite without more focused evidence.
- Idea 337 remains closed: no classified residual currently points first to a
  live scalar home in a callee-saved register missing frame preservation across
  a call.

Best next focused owner candidate: split the 9-case AArch64 scalar cast
machine-printer bucket. It has direct diagnostics, multiple representatives,
and a crisp semantic boundary: selected scalar cast nodes for sign/zero
extension reach the printer without a structured register source operand.
The runtime buckets are parked because they are heterogeneous and several
would need deeper localization before they are safe focused owners.

## Suggested Next

Delegate Step 3 (`Select The Next Focused Owner`) to turn the scalar cast
machine-printer candidate into a focused owner proposal with scope, non-goals,
representative tests, and proof ladder. Do not create or switch lifecycle state
inside the executor packet.

## Watchouts

- Do not implement under the umbrella inventory plan.
- Do not reopen closed owners through idea 337 from counts alone; use
  generated-code or diagnostic evidence to prove a current first bad fact.
- Treat timeout and output-storm cases as unsafe for broad reruns unless the
  delegated command includes timeout and stale-process cleanup.
- Do not change expectations, runner behavior, unsupported classifications, or
  CTest registration to improve backend-regex counts.
- The scalar-cast printer bucket is adjacent to closed idea 334; keep the scope
  on cast operand facts, not generic scalar arithmetic operand printing.

## Proof

Command run:

`git diff --check`

Result: passed. Existing classification input remains preserved at
`test_after.log`.
