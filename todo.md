Status: Active
Source Idea Path: ideas/open/540_rv64_object_select_edge_publication_helper_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Select And Publication Ownership

# Current Packet

## Just Finished

Activation created this executor-compatible skeleton for Step 1 of `plan.md`.

## Suggested Next

Supervisor should delegate Step 1: map select-source, publication move, predecessor-edge, and rejection-diagnostic helper ownership; record the first extraction target and deferred-helper rationale in this file.

## Watchouts

- Preserve prepared publication facts, fallback behavior, diagnostics, object bytes, tests, expectations, unsupported markers, and runtime contracts.
- Do not hide `fragment_for_prepared_instruction` fanout behind another broad dispatcher.
- Do not turn scalar helpers into select/publication catch-all ownership.
- Leave the existing untracked review artifact untouched.

## Proof

No validation run for lifecycle activation. First executor packet should use the supervisor-selected proof command and write the result to `test_after.log`.
