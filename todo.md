Status: Active
Source Idea Path: ideas/open/730_post_legacy_bir_shell_bootstrap.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Freeze the bounded schema and API checkpoint

# Current Packet

## Just Finished

- Step 2's first independent review blocked Step 3 and recorded three high,
  two medium, and one low schema/API finding in
  `review/730_step2_schema_checkpoint_review.md`.
- The checkpoint is now repaired: it defines repo-local C++17 `Result` and
  checked borrowed lookup representation, removes bootstrap block arguments and
  block insertion, freezes scoped builder capability/publication lifetime, and
  defines non-empty unique link-name merge/index/verifier semantics.
- Declaration order, verifier numbering, import mapping, packet boundaries, and
  deferred-feature prose were updated consistently with those repairs.

## Suggested Next

- Re-run an independent Step 2 review against the repaired checkpoint and the
  prior findings. Do not authorize Step 3A until that review is non-blocking.

## Watchouts

- Step 3 remains review-blocked despite the repairs; the checkpoint is pending
  re-review, not accepted implementation authority yet.
- Phi/block arguments and switch block insertion now have no bootstrap API.
- `ModuleBuilder` is non-copyable/non-movable, permits one synchronous scoped
  function edit, and cannot publish while a mutable capability is active.
- Full symbol storage remains deferred; the bounded link-name index is only the
  import-spine uniqueness/merge mechanism and never stable entity identity.

## Proof

- `git diff --check` — passed after repair.
- Focused forbidden-surface scan for `Expected`, `BlockArgument`,
  `add_block_argument`, and `create_block_before` — no matches.
- Checked proposed source/include paths: none is under `src/backend/legacy/`;
  occurrences of that path are prohibition statements only — passed.
- Documentation-only repair; no build/test run and no log rewrite.
