Status: Active
Source Idea Path: ideas/open/376_rv64_object_route_prepared_move_bundle_target_shapes.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Rejected Move Bundle

# Current Packet

## Just Finished

Lifecycle activation created `plan.md` and this executor-compatible `todo.md`
for Step 1.

## Suggested Next

Execute Step 1 from `plan.md`: audit the rejected prepared move bundle for
`src/20000217-1.c` using the post-idea-375 runner and case logs, then record
the first in-scope scalar integer move target shape or the exact out-of-scope
owner.

## Watchouts

- Do not reopen scalar compare/trunc lowering from idea 375.
- Do not infer move semantics from testcase names, hard-coded value ids, source
  syntax, instruction indexes, or the exact representative expression.
- Keep unsupported homes, widths, phases, and ambiguous bundles fail-closed with
  precise diagnostics.

## Proof

Lifecycle-only activation; no build required.
