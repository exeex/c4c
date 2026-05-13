Status: Active
Source Idea Path: ideas/open/200_hir_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Fence Lowerer Local, Param, Static, Label, And Signature Mirrors

# Current Packet

## Just Finished

Step 4 is complete. The remaining lowerer local const, label, local prototype,
and signature mirror audit found no unfenced semantic rendered-name fallback in
the owned surfaces. Local consts use source `TextId`/structured const-eval maps
before the retained no-metadata rendered map; local function prototypes resolve
through structured module declaration lookup before skipping the local alloca;
function-pointer signature mirrors already require source `TextId`/route-local
id authority or explicit rendered compatibility; and label names are route-local
block labels with no separate structured metadata fallback to fence.

## Suggested Next

Ask the plan owner to advance the active lifecycle to Step 5, then audit
HIR-to-LIR handoff compatibility tags for rendered owner/type/layout fallback.

## Watchouts

- The compatibility helper treats a valid source `TextId` as complete enough to
  reject name-only rendered compatibility; callers must insert an explicit
  `rendered_compat_*_text_ids` entry when a valid-`TextId` generated bridge is
  intentional. Generated bridges may use the HIR-canonical spelling `TextId`
  because parser-owned `TextId` values do not necessarily share the HIR
  link-name table.
- Static/global and local const bridges already have nearby focused tests; keep
  the next packet semantic rather than expectation-only.
- Labels remain plain `label_blocks`/`GotoStmt::target.user_name` route-local
  block bookkeeping; do not add a rendered compatibility layer there unless a
  future source-carrier design adds structured label metadata first.

## Proof

Passed: `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^frontend_hir_lookup_tests$"' > test_after.log 2>&1`

Proof log: `test_after.log`
