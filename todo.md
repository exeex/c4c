Status: Active
Source Idea Path: ideas/open/293_aarch64_side_effect_control_value_publication_authority.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Validate Boundary and Broader Sampling

# Current Packet

## Just Finished

Step 3 finished the focused starter subset by repairing `00183`
conditional-expression result publication while preserving `00164` and `00202`.
The AArch64 control-publication path now lowers prepared scalar `select`
results by comparing the selected predicate, materializing same-block scalar
producer values into non-overlapping registers, and publishing the selected
value into the prepared result register. The dispatch path also skips
coalesced out-of-SSA join-transfer source producers in the successor block when
the prepared predecessor parallel copy has already made source and destination
share the selected register, preventing stale recomputation from overwriting the
published result.

Generated-code evidence for `00183`: the final call argument now computes both
arms and selects into `%t21`'s prepared register `w13` before `printf` consumes
it:

```asm
ldr w10, [sp]
mov w13, #3
mul w13, w10, w13
csel w13, w9, w13, lt
mov x0, x20
mov x1, x13
```

The focused subset now passes all three starter reps. `00183` prints the final
five loop iterations as `15`, `18`, `21`, `24`, `27`, and the proof preserved
the previously passing `00164` and `00202`.

## Suggested Next

Continue Step 3 or ask the supervisor whether the starter subset is ready to
move to Step 4 boundary sampling with `00169` plus the preserved starter reps.

## Watchouts

- The repair remains tied to explicit scalar control/select publication and
  coalesced out-of-SSA join-transfer authority. Do not broaden this into
  enabling every `PreparedMovePhase::BeforeInstruction` bundle; that previously
  regressed the first-six-line `00164` contract.
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
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan -R 'c_testsuite_aarch64_backend_src_(00164|00183|00202)_c$' -j 4 --timeout 5 --output-on-failure; } > test_after.log 2>&1
```

Result: passed. `test_after.log` shows `100% tests passed, 0 tests failed out
of 3` for `00164`, `00183`, and `00202`.

Stale-process check:

```sh
pgrep -af '^/workspaces/c4c/build-aarch64-scan/c_testsuite_aarch64_backend/' || true
```

Result: no generated runtime process remained.

Additional check: `git diff --check` passed.

Log paths:

- `test_after.log`
