# RV64 20000622-1 Foo Logical Select Runtime Abort

Status: Open
Type: Capability repair
Parent: `ideas/closed/577_rv64_20000622_1_runtime_abort_after_call_lowering.md`
Related: `ideas/open/573_rv64_select_phi_select_lowering.md`
Owning Layer: RV64 prepared select-chain and logical-condition publication

## Goal

Classify and repair the later `foo` logical/select runtime family exposed after
the `src/20000622-1.c` RV64 object route advanced past the repaired `baz`
formal pointer `ptrtoint` materialization bug.

## Why This Exists

Idea 577 repaired the first classified family: `baz` now materializes
`d = (long)c` from incoming `a2`, preserves the loaded local in `s2` across
`bar`, and passes `s2` as `foo` argument 0. The representative still reports
`[RV64_BACKEND_RUNTIME_MISMATCH] clang_exit=0 c4c_exit=Subprocess aborted`,
but the current artifacts classify the remaining failure as a later
logical/select publication family inside `foo`, not the old `baz` argument
source selection problem.

Evidence:

- `build/agent_state/577_rv64_20000622_1_runtime_abort_after_call_lowering/src_20000622-1.c/classification-summary.txt`
- `build/agent_state/577_rv64_20000622_1_runtime_abort_after_call_lowering/src_20000622-1.c/dump-prepared-bir.txt`
- `build/agent_state/577_rv64_20000622_1_runtime_abort_after_call_lowering/src_20000622-1.c/c4c.bin.disasm`
- `build/agent_state/577_rv64_20000622_1_runtime_abort_after_call_lowering/src_20000622-1.c/clang.bin.disasm`

The prepared BIR for `foo` contains `%t13 = bir.select ne i64 %p.a, 12, i32
1, %t9` and `%t24 = bir.select ne i32 %t13, 0, i32 1, %t20`, with prepared
select-chain materializations recorded for `%t13` and `%t24`. The same dump
records missing or unsupported select carrier alias authority for the incoming
binary and immediate sources feeding those select results.

## In Scope

- Reproduce the current `src/20000622-1.c` RV64 object-route runtime abort from
  the 577 artifact state.
- Classify whether the first bad owner is select materialization, select
  carrier alias publication, logical short-circuit lowering, or a related
  prepared-value publication rule.
- Add focused backend or runtime coverage for the `foo`-style scalar logical
  select chain without depending on the representative filename.
- Repair the underlying RV64 object-route semantics so `foo(12, 1, 11)` does
  not take the abort path because of inverted or stale select publication.
- Preserve the fixed 577 `baz` route: `a2 -> ptrtoint -> local -> s2 -> foo`
  argument 0.

## Out Of Scope

- Reopening the 577 `baz` formal pointer `ptrtoint` materialization repair
  unless fresh evidence shows it regressed.
- Reopening ordinary same-module `CallInst` fallback work from 572.
- Generic select or phi-select cleanup unrelated to the `foo` logical
  condition runtime family.
- Expectation rewrites, unsupported-marker changes, allowlist edits, runtime
  comparison contract changes, or gcc_torture runner changes.
- Filename-specific handling for `src/20000622-1.c`, exact block names, exact
  generated value names, or exact disassembly offsets.

## Acceptance Criteria

- The first `foo` logical/select bad owner is recorded with prepared-BIR,
  object, or runtime artifacts.
- Focused coverage proves the repaired scalar logical/select publication shape
  without depending on the exact representative filename.
- The `src/20000622-1.c` object-route rerun no longer aborts because
  `foo(12, 1, 11)` takes the failure path through stale, inverted, or missing
  select publication.
- Existing 572 same-module call/result coverage and the 577 `baz` ptrtoint
  materialization behavior remain intact.

## Reviewer Reject Signals

- Reject a slice that handles only `src/20000622-1.c`, function `foo`, `%t13`,
  `%t24`, `logic.end.7`, `logic.end.18`, or exact emitted offsets from the
  saved disassembly.
- Reject expectation rewrites, unsupported-marker edits, allowlist changes,
  runtime comparison changes, or gcc_torture runner changes claimed as the
  repair.
- Reject a route that claims 577-style progress while regressing the fixed
  `baz` path where incoming `a2` is materialized, preserved through `bar`, and
  passed as `foo` argument 0.
- Reject a route that treats the remaining abort as same-module call argument
  publication or `bar(a, 1)` result publication without new artifacts proving
  that classification.
- Reject helper renames, diagnostic relabeling, or classification-only changes
  claimed as capability progress.
- Reject broad RV64 select rewrites that do not include focused proof for the
  observed logical/select runtime condition family.
