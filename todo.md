Status: Active
Source Idea Path: ideas/open/293_aarch64_side_effect_control_value_publication_authority.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Repair Control-Selected Expression Publication

# Current Packet

## Just Finished

Step 3 reduced the dirty slice to the first same-block scalar compare
publication primitive for AArch64 control values and fixed the reviewer-found
scratch-clobber hazard in that path. Compare binaries now materialize through
selected `cmp`/`cset` assembler nodes when their operands have authoritative
same-block sources: immediates, same-block emitted scalar registers, or
same-block prepared frame-slot loads. Dispatch records the emitted compare
result in `BlockScalarLoweringState`, so later call-boundary argument moves can
consume the freshly materialized register instead of a stale logical home. The
compare materializer now carries the actual LHS register/scratch as occupied
while materializing RHS, so memory/memory compares cannot clobber the loaded
LHS before `cmp`.

Generated `src/00164.c` evidence now shows comparison call arguments published
before `printf`:

```asm
ldr w9, [sp]
ldr w10, [sp]
cmp w9, w10
cset w13, eq
ldr w9, [sp]
ldr w10, [sp, #4]
cmp w9, w10
cset w9, eq
mov x0, x21
mov x1, x13
mov w2, w9
bl printf
...
ldr w9, [sp]
ldr w10, [sp]
cmp w9, w10
cset w13, ne
...
cmp w9, w10
cset w9, ne
```

The proof still fails overall and does not improve the strict pass count over
`test_before.log` (both are 1/3 passing). `00202` remains passing and the first
six `00164` output lines are preserved (`134`, `134`, `0`, `1`, `1`, `1`).
`00164` also now prints the equality/inequality pair lines correctly
(`1, 0` and `0, 1`). The remaining first bad value is still the `%t106`
control-selected chain after `%t104`: generated assembly computes `%t103`
with `orr`, but the following `printf` still consumes stale `x13` for `%t106`.
The incomplete select-publication attempt was removed from the dirty diff. A
terminator-move scheduling probe was also rejected because the prepared
parallel-copy source for `%t106` is `%t104`, while `%t104` is a value in
successor block `logic.end.100`, not an already-materialized predecessor value.
The precise remaining boundary is therefore select materialization with
predecessor/edge authority, not same-block compare publication.

## Suggested Next

Continue Step 3 by adding an edge-aware select publication primitive for
prepared `bir.select` trees. The next slice should consume prepared
join-transfer/parallel-copy authority for predecessor-carried select operands,
then materialize `%t106` without reading arbitrary register homes from paths
where the source value was not defined.

Proposed proof command:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan -R 'c_testsuite_aarch64_backend_src_(00164|00183|00202)_c$' -j 4 --timeout 5 --output-on-failure; } > test_after.log 2>&1
```

## Watchouts

- Do not lower select operands from plain prepared register homes unless a
  same-block emitted value, same-block prepared memory access, or explicit
  predecessor-edge transfer proves the source is authoritative on the current
  path. A broader attempt broke the first-six-line `00164` progress.
- `%t104 = bir.ne i32 %t103, 0` belongs to the repaired same-block compare
  path when `%t103` is emitted by the preceding `orr`; `%t106` remains blocked
  by select-tree predecessor authority.
- The concrete missing prepared authority is the
  `PreparedJoinTransfer` for `logic.end.100 result=%t106` with
  `carrier=select_materialization`, `ownership=authoritative_branch_pair`, and
  predecessor-terminator `PreparedParallelCopyBundle`s for
  `logic.rhs.end.99 -> logic.end.100` (`%t104 -> %t106`) and
  `logic.skip.98 -> logic.end.100` (`0 -> %t106`). The current owned diff does
  not yet have an edge-aware way to materialize the `%t104` source before that
  copy.
- `00183` is still unchanged by this slice and remains the broader
  conditional-expression target for Step 3 after the `00164` select boundary is
  owned.
- Do not touch expected outputs, allowlists, unsupported classifications,
  timeout policy, runner behavior, CTest registration, or build/test
  infrastructure.

## Proof

Ran the delegated focused proof exactly:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan -R 'c_testsuite_aarch64_backend_src_(00164|00183|00202)_c$' -j 4 --timeout 5 --output-on-failure; } > test_after.log 2>&1
```

Result: failed overall, 1/3 passing, matching `test_before.log` by pass count.
`00202` passed. `00164` and `00183` remain `RUNTIME_MISMATCH`. `00164`
preserved the first six expected outputs and advanced the same-block comparison
publication lines; the remaining bad values are the stale `%t106` select result
and later arithmetic call-result values. The current `00164` actual output
starts `134`, `134`, `0`, `1`, `1`, `1`, then stale `%t106` values before the
now-correct `1, 0` and `0, 1` comparison lines. The reviewer scratch-clobber
fix is covered by the rebuilt `c4cll` and generated-code evidence now showing
distinct LHS/RHS scratch registers for memory/memory compares.

Stale-process check:

```sh
pgrep -af '^/workspaces/c4c/build-aarch64-scan/c_testsuite_aarch64_backend/' || true
```

Result: no generated runtime process remained.

Additional check: `git diff --check` passed.

Log paths:

- `test_after.log`
