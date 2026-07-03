Status: Active
Source Idea Path: ideas/open/578_rv64_20000622_1_foo_logical_select_runtime_abort.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify The First Bad Owner

# Current Packet

## Just Finished

- Step 2 Classify The First Bad Owner completed for `src/20000622-1.c`.
- First bad owner: RV64 select carrier alias publication / prepared-value
  publication for the binary source feeding a select-materialized logical OR
  result, not logical short-circuit topology and not the old 577 `baz` route.
- First bad edge: in `foo`, `logic.rhs.end.6 -> logic.end.7` publishes
  incoming `%t9` into destination `%t13`. Prepared BIR says `%t9` is
  `bir.ne i64 %p.b, 1`, and the prepared parallel copy records
  `%t9 -> %t13` with `carrier=select_materialization`.
- Object evidence: current c4c disassembly for the `a == 12` path emits
  `mv s2,a1; mv s1,s2`, so `%t13` receives raw `b`. For `foo(12, 1, 11)`,
  that makes `%t13 == 1`, the branch on `%t13 != 0` takes the failure/skip
  path, `%t24` is forced to `1`, and `abort` is reached.
- Expected behavior: `%t13` should receive `%t9 = (b != 1)`, which is `0`
  for `b == 1`, then evaluate `c != 11`. Clang's disassembly compares `b`
  directly with `1` and continues to the `c == 11` check.
- Distinction from adjacent routes: current c4c `baz` still materializes
  incoming `a2` through stack/`s2` and calls `foo` with `a0=s2`,
  `a1=bar` result, and `a2=preserved b`; the first wrong value occurs inside
  `foo` after entry. This is also not 572 same-module call fallback work
  because `bar` is called and its result is loaded into `a1` before `foo`.

## Suggested Next

- Delegate Step 3: Add Focused Coverage for the generic scalar logical-OR
  select publication shape where the false edge of an earlier condition must
  publish a binary compare result, not the raw compare operand, into a
  select-materialized carrier.

## Watchouts

- Do not treat expectation rewrites, unsupported-marker edits, allowlist changes,
  runtime comparison changes, or filename-specific handling as progress.
- Preserve the fixed 577 `baz` route: incoming `a2` materialized through
  `ptrtoint`, preserved across `bar`, and passed as `foo` argument 0.
- The repair route should target carrier alias/prepared-value publication for
  select materialization sources. A repair that only rewrites branch topology,
  recognizes `foo`, or patches `%t13`/`%t24` by name would be testcase overfit.
- The second select `%t24` has the same missing/unsupported carrier authority
  pattern, but the first runtime-wrong owner is already visible at `%t13`:
  raw `b` is published where `%t9 = (b != 1)` is required.

## Proof

- Proof log: `test_after.log`.
- Refreshed prepared-BIR command returned rc `0` and wrote:
  `build/agent_state/578_rv64_20000622_1_foo_logical_select_runtime_abort/src_20000622-1.c/dump-prepared-bir.txt`.
- Classification transcript in `test_after.log` includes the refreshed
  prepared-BIR select chain, join transfers, carrier alias authority rows,
  current c4c and clang `foo` disassembly snippets, current c4c `baz` call
  evidence, saved 577 `baz -> foo` publication rows, and the owner conclusion.
- No build or implementation proof was run for this classification-only packet.
