Status: Active
Source Idea Path: ideas/open/293_aarch64_side_effect_control_value_publication_authority.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Validate Boundary and Broader Sampling

# Current Packet

## Just Finished

Step 4 repaired the `00169` boundary case while preserving the passing starter
reps `00164`, `00183`, and `00202`. The call-boundary dispatcher now retargets
prepared call moves to the freshest emitted scalar source when one exists, keeps
the ABI destination view width-compatible with that fresh source, and publishes
only source-conflicting argument moves before a call-site address materialization
reuses the same register. This prevents a live scalar call argument from being
read out of a stale prepared home after the address materialization clobbers the
last emitted register.

Generated-code evidence for `00169`: the middle loop value `%t20` is loaded
into `w20`, published to the variadic `printf` argument register `w2`, and only
then is `x20` reused for the format-string address:

```asm
ldr w13, [sp]
ldr w20, [sp, #4]
mov w2, w20
adrp x20, .str0
add x20, x20, :lo12:.str0
mov x0, x20
mov w1, w13
ldr w3, [sp, #8]
bl printf
```

The focused boundary subset improved from the baseline `3/4` to `4/4`: `00169`
now passes, and `00164`, `00183`, and `00202` remained passing.

## Suggested Next

Supervisor should review and commit this Step 4 slice, then decide whether Step
4 needs broader sampling beyond the four delegated reps or whether the runbook
is ready for lifecycle review.

## Watchouts

- The repair remains tied to explicit scalar control/select publication and
  coalesced out-of-SSA join-transfer authority. Do not broaden this into
  enabling every `PreparedMovePhase::BeforeInstruction` bundle; that previously
  regressed the first-six-line `00164` contract.
- The new call-boundary ordering is intentionally narrow: it hoists only
  argument moves whose fresh emitted source register aliases a pending
  call-site address materialization result, and it excludes the address value
  itself. Avoid turning this into wholesale reordering of the prepared
  call-argument bundle.
- The implemented edge hook currently handles the bounded shape needed by
  `%t106`: an out-of-SSA predecessor block-entry bundle with a named scalar
  compare source defined in the successor join prefix, and only when source and
  destination prepared homes name the same register. It now also allows prepared
  load-source operands for the edge-computed compare source. If the next stale
  value needs non-coalesced edge publication, add an explicit prepared-register
  move after source lowering rather than assuming the source home is the
  destination.
- The new select publication path handles scalar same-block binary producers
  for selected values and relies on `csel` after a predicate `cmp`; inserted
  materialization must not clobber condition flags before `csel`.
- Prepared memory access instruction indexes can lag the compacted prepared BIR
  block indexes; the scalar fallback now reloads by prepared result value name
  when ordinary memory lowering misses an un-emitted register-home load.
- Do not touch expected outputs, allowlists, unsupported classifications,
  timeout policy, runner behavior, CTest registration, or build/test
  infrastructure.

## Proof

Ran the delegated proof exactly:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan -R 'c_testsuite_aarch64_backend_src_(00164|00169|00183|00202)_c$' -j 4 --timeout 5 --output-on-failure; } > test_after.log 2>&1
```

Result: passed. `test_after.log` shows `100% tests passed, 0 tests failed out
of 4` for `00164`, `00169`, `00183`, and `00202`. This is a strict
pass-count improvement over `test_before.log`, which showed `00169` failing
with the second printed field clobbered while the other three reps passed.

Stale-process check:

```sh
pgrep -af '^/workspaces/c4c/build-aarch64-scan/c_testsuite_aarch64_backend/' || true
```

Result: no generated runtime process remained.

Additional check: `git diff --check` passed.

Log paths:

- `test_after.log`
