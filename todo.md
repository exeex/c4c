Status: Active
Source Idea Path: ideas/open/293_aarch64_side_effect_control_value_publication_authority.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Repair Control-Selected Expression Publication

# Current Packet

## Just Finished

Step 3 repaired the remaining `00164` arithmetic publication reuse after the
fixed logical/select boundary. The call-boundary path now materializes same-block
scalar ALU producers for named call arguments when the authoritative value has
not yet been emitted, then retargets the synthetic materialization result to the
prepared result register named by the value home. This keeps the repair tied to
call-argument publication instead of enabling every `BeforeInstruction` move.

Generated-code evidence for `00164`: after the fixed eleventh print for
`a != b && c != d`, both `a + b * c / f` call arguments now compute the final
add into `%t162`/`%t171`'s prepared register `w21` before `printf` consumes it,
while `x20` remains the format-string register:

```asm
ldr w10, [sp]
add w21, w10, w9
mov x0, x20
mov x1, x21
```

The focused subset still fails overall because `00183` remains the known
runtime mismatch, while `00164` and `00202` now pass. `00164` now prints all
expected lines, including the first eleven preserved lines, compare pair
`1, 0` / `0, 1`, and arithmetic lines 12 and 13 as `1916`, `1916`.

## Suggested Next

Continue Step 3 with `00183`, which remains the only failing test in the focused
starter subset. The next packet should repair the conditional-expression result
publication that still prints `25`, `36`, `49`, `64`, `81` for the final five
loop iterations instead of `15`, `18`, `21`, `24`, `27`, while preserving the
now-passing `00164` and `00202`.

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
- A broad attempt to make scalar ALU register conversion prefer explicit
  register names over placements fixed `00164` locally but regressed `00183` by
  computing the loop increment in `w21` while the store still consumed `w20`.
  The landed patch keeps the explicit-name override local to synthetic
  call-argument materialization.
- Do not touch expected outputs, allowlists, unsupported classifications,
  timeout policy, runner behavior, CTest registration, or build/test
  infrastructure.

## Proof

Ran the delegated proof exactly:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan -R 'c_testsuite_aarch64_backend_src_(00164|00183|00202)_c$' -j 4 --timeout 5 --output-on-failure; } > test_after.log 2>&1
```

Result: failed overall at the focused scan subset with strict pass-count
improvement over `test_before.log`. The scan subset is now 2/3 passing:
`00164` passed, `00202` passed, and `00183` remains `RUNTIME_MISMATCH`.
`test_before.log` was 1/3 passing with `00164` printing stale arithmetic lines
`2`, `2`; `test_after.log` has `00164` passing with lines 12 and 13 fixed to
`1916`, `1916`. `00183` remains unchanged at actual `0`, `1`, `4`, `9`, `16`,
`25`, `36`, `49`, `64`, `81`.

Stale-process check:

```sh
pgrep -af '^/workspaces/c4c/build-aarch64-scan/c_testsuite_aarch64_backend/' || true
```

Result: no generated runtime process remained.

Additional check: `git diff --check` passed.

Log paths:

- `test_after.log`
