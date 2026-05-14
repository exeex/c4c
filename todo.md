Status: Active
Source Idea Path: ideas/open/228_aarch64_module_phoenix_drafts_to_implementation.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Implement One Lowering Family At A Time

# Current Packet

## Just Finished

Step 5: Implement One Lowering Family At A Time cleaned up the stale
`src/backend/mir/aarch64/module/*.md` replacement drafts and stage notes after
their implemented surfaces had moved into compiled module/codegen C++.

`plan.md` no longer points active executors at deleted `module/*.md` files; its
active read/target list now references the current `module/module.*` and
`codegen/*.cpp/.hpp` implementation surfaces.

## Suggested Next

Supervisor can review and commit this docs/lifecycle cleanup, then proceed to
the compatibility projection split or the next route-review packet.

## Watchouts

- The removed stage/draft notes were stale route scaffolding, not canonical
  source intent; durable source intent remains in `ideas/open/228...` and the
  parent `ideas/open/224...`.
- Review checkpoint: the codegen headers introduced during the extraction
  remain target-private helper surfaces used by focused tests, not stable
  module public API.
- No implementation, lowering expectations, or testcase contracts were changed
  in this packet.

## Proof

Docs/lifecycle-only cleanup. Validation is reference hygiene:
`find src/backend/mir/aarch64/module -maxdepth 1 -name '*.md' -print` and
`rg -n 'src/backend/mir/aarch64/module/.*\\.md|module/(module|module\\.hpp|stage2_review_layout|stage2_to_stage3_handoff|stage3_draft_review)\\.md' plan.md ideas/open tests`.
