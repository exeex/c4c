Status: Active
Source Idea Path: ideas/open/386_aarch64_calls_mechanical_split.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Map Calls Dependencies

# Current Packet

## Just Finished

Lifecycle activation created `plan.md` and this executor-compatible `todo.md`
for Step 1.

## Suggested Next

Start Step 1 by loading `.codex/skills/c4c-clang-tools/SKILL.md`, running the
`calls.cpp` function-signature query, and querying callers or callees for
representative functions in the proposed extraction clusters. Select the first
mechanical extraction packet and record the chosen cluster here before code
edits begin.

## Watchouts

- Unrelated dirty implementation/test files were already present when this
  lifecycle state was activated; do not revert or sweep them into a lifecycle
  commit.
- Keep this idea behavior-preserving and reject testcase-shaped shortcuts,
  expectation downgrades, or backend capability changes.
- Record awkward dependencies here first instead of editing the source idea.

## Proof

Activation is lifecycle-only; no build or test proof was run.
