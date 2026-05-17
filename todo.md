Status: Active
Source Idea Path: ideas/open/271_aarch64_variadic_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Delete The Markdown Shard

# Current Packet

## Just Finished

Completed Plan Step 4, `Delete The Markdown Shard`.

Deleted `src/backend/mir/aarch64/codegen/variadic.md`. Cleaned active markdown
references that still treated the deleted shard as an implementation or bring-up
authority:

- `CLASSIFICATION_INDEX.md` no longer lists `codegen/variadic.md` as a
  target-ABI candidate and now points variadic ownership at compiled
  `codegen/variadic.{hpp,cpp}` plus prepared-call and AAPCS64 contracts.
- `BACKEND_CASE_BRINGUP_MATRIX.md` keeps the aggregate/variadic public cases
  blocked, but routes future variadic ownership through compiled
  `variadic.cpp` / `variadic.hpp`, `AAPCS64_CALL_RETURN_FRAME_CONTRACT.md`, and
  prepared-call variadic fields instead of the deleted shard.
- `codegen/README.md` preserves the historical module-surface note while
  recording that the legacy variadic markdown shard is deleted and compiled
  ownership is `variadic.hpp` / `variadic.cpp`.

This was a documentation/lifecycle deletion only. Public support statuses and
test expectations were not weakened, and no implementation source files or
tests were changed.

## Suggested Next

Step 4 deletion is ready for supervisor review and commit. The next coherent
packet should be chosen by the supervisor or plan owner after deciding whether
the active runbook is exhausted by this shard deletion.

## Watchouts

Preserve the explicit deferral of full variadic function-entry behavior. The
deleted markdown shard is no longer an active implementation authority; future
variadic work should use the compiled helper boundary and accepted ABI/prepared
contracts instead of reviving the shard.

## Proof

No build or test proof was run; the delegated packet was docs/lifecycle deletion
only and explicitly required no build unless compiled files changed. No compiled
files changed, so no build was required. `test_after.log` was not touched.
