Status: Active
Source Idea Path: ideas/open/293_aarch64_side_effect_control_value_publication_authority.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Repair Control-Selected Expression Publication

# Current Packet

## Just Finished

Step 3 materially advanced the next `00164` stale publication after the compare
pair by repairing edge-local control compare publication for
`a != b && c != d`. The predecessor parallel-copy source hook now asks scalar
control publication to allow prepared-load-source operands only for the
synthetic edge lowering path, so `%t149 = c != d` can be computed on the
`logic.rhs.end.145 -> logic.end.146` edge without enabling all
`BeforeInstruction` moves or broadening ordinary same-block control publication.

Generated-code evidence for `00164`: after the `a != a, a != b` compare pair,
the true edge for `a != b && c != d` now reloads `c` and `d`, compares them, and
publishes `%t153` in `w13` before the eleventh `printf`:

```asm
ldr w9, [sp, #8]
ldr w10, [sp, #12]
cmp w9, w10
cset w13, ne
```

The focused subset still fails overall with known remaining `00164` and `00183`
runtime mismatches, while `00202` passes. `00164` preserves the first ten lines:
`134`, `134`, `0`, `1`, `1`, `1`, `1`, `46`, `1, 0`, `0, 1`. The eleventh line
now prints the expected `1`. The next `00164` stale publication is the arithmetic
reuse after that boundary: lines 12 and 13 now print `2`, `2` instead of
`1916`, `1916`.

## Suggested Next

Continue Step 3 at the next `00164` stale scalar/call publication after the
fixed `a != b && c != d` boundary. The next packet should target lines 12 and
13, where `a + b * c / f` now prints stale call/control state as `2`, `2`
instead of `1916`, `1916`, while preserving the first eleven `00164` lines,
`00202`, and the compare pair.

Proposed proof command:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan -R 'c_testsuite_aarch64_backend_src_(00164|00183|00202)_c$' -j 4 --timeout 5 --output-on-failure; } > test_after.log 2>&1
```

## Watchouts

- A broad attempt to emit every `PreparedMovePhase::BeforeInstruction` bundle
  regressed the first-six-line `00164` contract. Keep the next slice tied to
  explicit predecessor parallel-copy or join-transfer authority.
- A broad attempt to allow prepared-load-source operands for all control
  publication fixed line 11 but regressed line 3 from `0` to `1`. The landed
  path deliberately gates that reload behind the synthetic edge-publication
  call from `lower_predecessor_select_parallel_copy_sources`.
- The implemented edge hook currently handles the bounded shape needed by
  `%t106`: an out-of-SSA predecessor block-entry bundle with a named scalar
  compare source defined in the successor join prefix, and only when source and
  destination prepared homes name the same register. It now also allows prepared
  load-source operands for the edge-computed compare source. If the next stale
  value needs non-coalesced edge publication, add an explicit prepared-register
  move after source lowering rather than assuming the source home is the
  destination.
- Prepared memory access instruction indexes can lag the compacted prepared BIR
  block indexes; the scalar fallback now reloads by prepared result value name
  when ordinary memory lowering misses an un-emitted register-home load.
- `00183` is still unchanged by this slice and remains the broader
  conditional-expression target for Step 3 after the `00164` select boundary is
  owned.
- Do not touch expected outputs, allowlists, unsupported classifications,
  timeout policy, runner behavior, CTest registration, or build/test
  infrastructure.

## Proof

Ran the delegated proof exactly:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan -R 'c_testsuite_aarch64_backend_src_(00164|00183|00202)_c$' -j 4 --timeout 5 --output-on-failure; } > test_after.log 2>&1
```

Result: failed overall at the focused scan subset. The scan subset remains 1/3
passing: `00202` passed; `00164` and `00183` remain `RUNTIME_MISMATCH`. `00164`
materially advanced: the first ten lines remain `134`, `134`, `0`, `1`, `1`,
`1`, `1`, `46`, `1, 0`, `0, 1`, and the eleventh line now prints the expected
`1`. The next stale `00164` boundary is lines 12 and 13, expected `1916`,
`1916`, actual `2`, `2`. `00183` remains unchanged at `0`, `1`, `4`, `9`,
`16`, `25`, `36`, `49`, `64`, `81`.

Stale-process check:

```sh
pgrep -af '^/workspaces/c4c/build-aarch64-scan/c_testsuite_aarch64_backend/' || true
```

Result: no generated runtime process remained.

Additional check: `git diff --check` passed.

Log paths:

- `test_after.log`
