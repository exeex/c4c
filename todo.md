Status: Active
Source Idea Path: ideas/open/578_rv64_20000622_1_foo_logical_select_runtime_abort.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Rehydrate Evidence And Reproduce

# Current Packet

## Just Finished

- None. Plan activated from source idea 578.

## Suggested Next

- Delegate Step 1: Rehydrate Evidence And Reproduce.

## Watchouts

- Do not treat expectation rewrites, unsupported-marker edits, allowlist changes,
  runtime comparison changes, or filename-specific handling as progress.
- Preserve the fixed 577 `baz` route: incoming `a2` materialized through
  `ptrtoint`, preserved across `bar`, and passed as `foo` argument 0.

## Proof

- Lifecycle-only activation; no code proof run.
