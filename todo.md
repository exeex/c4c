Status: Active
Source Idea Path: ideas/open/228_aarch64_module_phoenix_drafts_to_implementation.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Implement One Lowering Family At A Time

# Current Packet

## Just Finished

Step 5: Implement One Lowering Family At A Time cleaned up the stale
`src/backend/mir/aarch64/module/*.cpp.md` replacement drafts after their
implemented surfaces had moved into compiled module/codegen C++.

`plan.md` no longer points active executors at deleted `module/*.cpp.md` files;
its active read/target list now references the current `module/module.*` and
`codegen/*.cpp/.hpp` implementation surfaces.

## Suggested Next

Supervisor can review and commit this docs/lifecycle cleanup, then proceed to
the compatibility projection split or the next route-review packet.

## Watchouts

- Historical stage notes under `src/backend/mir/aarch64/module/` may still
  mention the removed draft filenames as archive context; active `plan.md` no
  longer depends on them.
- Review checkpoint: the codegen headers introduced during the extraction
  remain target-private helper surfaces used by focused tests, not stable
  module public API.
- No implementation, lowering expectations, or testcase contracts were changed
  in this packet.

## Proof

Docs/lifecycle-only cleanup. Validation is reference hygiene:
`rg -n 'module/(module|function_traversal|operand_resolution|instruction_lowering|branch_control_lowering|call_lowering|public_assembly_bridge|compatibility_projection)\\.cpp\\.md|src/backend/mir/aarch64/module/.*\\.cpp\\.md' plan.md todo.md ideas/open tests`.
