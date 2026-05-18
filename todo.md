Status: Active
Source Idea Path: ideas/open/293_aarch64_side_effect_control_value_publication_authority.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Repair Control-Selected Expression Publication

# Current Packet

## Just Finished

Step 3 repaired the regression introduced by `27a97e2ac` where
`backend_aarch64_machine_printer` failed the selected scalar fail-closed
contract for out-of-range add immediates. The scalar add/sub/bitwise printer now
restores the expected plain `#imm` diagnostic for non-encodable immediates and
the precise `sub` diagnostic for immediate-LHS/register-RHS shapes. This keeps
the common scalar printer rejecting unprintable selected nodes without changing
the Step 3 compare/control publication helper or weakening its same-block
compare publication behavior.

The delegated proof now passes the repaired internal regression and the broader
`^backend_aarch64_` suite. The focused c-testsuite subset still fails overall
with the already-known `00164`/`00183` runtime mismatches, while `00202`
passes, `00164` still preserves the first six output lines (`134`, `134`, `0`,
`1`, `1`, `1`), and the compare pair improvement remains visible as `1, 0` and
`0, 1`.

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

- The delegated proof command starts with `ctest` in `build`; after editing
  `alu.cpp`, the internal test binary had to be refreshed with
  `cmake --build build --target backend_aarch64_machine_printer_test` before the
  proof could exercise this slice.
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

Prerequisite stale-binary refresh:

```sh
cmake --build build --target backend_aarch64_machine_printer_test
```

Then ran the delegated proof exactly:

```sh
{ ctest --test-dir build -R '^backend_aarch64_machine_printer$' --output-on-failure && ctest --test-dir build -R '^backend_aarch64_' -j 8 --output-on-failure && cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan -R 'c_testsuite_aarch64_backend_src_(00164|00183|00202)_c$' -j 4 --timeout 5 --output-on-failure; } > test_after.log 2>&1
```

Result: failed overall at the final focused scan subset. The internal
`backend_aarch64_machine_printer` regression passed, and the broader
`^backend_aarch64_` suite passed 27/27. The scan subset remained 1/3 passing:
`00202` passed; `00164` and `00183` remain `RUNTIME_MISMATCH`. `00164` actual
output starts `134`, `134`, `0`, `1`, `1`, `1`, then stale select-chain values,
and still includes the repaired compare pair lines `1, 0` and `0, 1`.

Stale-process check:

```sh
pgrep -af '^/workspaces/c4c/build-aarch64-scan/c_testsuite_aarch64_backend/' || true
```

Result: no generated runtime process remained.

Additional check: `git diff --check` passed.

Log paths:

- `test_after.log`
