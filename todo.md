Status: Active
Source Idea Path: ideas/open/293_aarch64_side_effect_control_value_publication_authority.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Repair Control-Selected Expression Publication

# Current Packet

## Just Finished

Step 3 materially advanced the `00164` `%t106` select publication boundary by
using the prepared out-of-SSA parallel-copy edge authority for
`logic.rhs.end.99 -> logic.end.100`. The AArch64 dispatch now recognizes a
predecessor-terminator select-materialization copy whose source is a named
same-successor scalar compare, duplicates the required successor-prefix scalar
definition on that predecessor edge, and publishes the compare into the
prepared destination register before branching to the join. This avoids reading
the arbitrary `%t104` register home in the predecessor and leaves unrelated
before-instruction move bundles disabled.

Reviewer refinement: the helper now proves the selected source home and the
parallel-copy destination home are coalesced to the same prepared register
before accepting the edge publication. It also clears any dependency-prefix
lowering if the selected source compare itself cannot be lowered, so the helper
does not return a partial predecessor dependency without the selected source.

Generated-code evidence for `00164`: `.LBB89_36` now emits the prepared
`%t104 -> %t106` edge source before the branch to `.LBB89_37`:

```asm
ldr w9, [sp, #20]
ldr w10, [sp, #20]
orr w9, w9, w10
str w9, [sp, #140]
cmp w9, #0
cset w13, ne
```

The focused subset still fails overall with known remaining `00164` and `00183`
runtime mismatches, while `00202` passes. `00164` now preserves and advances the
front of the output to `134`, `134`, `0`, `1`, `1`, `1`, `1`; the first-six-line
contract is preserved, and the compare pair remains `1, 0` and `0, 1`.

## Suggested Next

Continue Step 3 at the next `00164` stale select-chain use after `%t106`. The
next packet should extend the same prepared predecessor-edge publication model
to the later `00164` join value that currently prints the eighth line as a
stale register value instead of `46`, without enabling all
`BeforeInstruction` moves globally.

Proposed proof command:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan -R 'c_testsuite_aarch64_backend_src_(00164|00183|00202)_c$' -j 4 --timeout 5 --output-on-failure; } > test_after.log 2>&1
```

## Watchouts

- A broad attempt to emit every `PreparedMovePhase::BeforeInstruction` bundle
  regressed the first-six-line `00164` contract. Keep the next slice tied to
  explicit predecessor parallel-copy or join-transfer authority.
- The implemented edge hook currently handles the bounded shape needed by
  `%t106`: an out-of-SSA predecessor block-entry bundle with a named scalar
  compare source defined in the successor join prefix, and only when source and
  destination prepared homes name the same register. If the next stale value
  needs non-coalesced edge publication, add an explicit prepared-register move
  after source lowering rather than assuming the source home is the destination.
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
advanced from the baseline first stale `%t106` line to actual output starting
`134`, `134`, `0`, `1`, `1`, `1`, `1`, then a later stale select-chain value;
the compare pair lines remain `1, 0` and `0, 1`.

Additional validation:

```sh
ctest --test-dir build -R '^backend_aarch64_' -j 8 --output-on-failure > /tmp/c4c_aarch64_side_effect_step3_edge_select_backend_aarch64.log 2>&1
```

Result: passed 27/27.

Stale-process check:

```sh
pgrep -af '^/workspaces/c4c/build-aarch64-scan/c_testsuite_aarch64_backend/' || true
```

Result: no generated runtime process remained.

Additional check: `git diff --check` passed.

Log paths:

- `test_after.log`
- `/tmp/c4c_aarch64_side_effect_step3_edge_select_backend_aarch64.log`
