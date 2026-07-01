Status: Active
Source Idea Path: ideas/open/422_bir_semantic_producer_high_impact_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Current Semantic Producer Failure Evidence

# Current Packet

## Just Finished

Activated idea 422, `BIR Semantic Producer High-Impact Cleanup`, as the next
concrete source idea.

Selection rationale:

- `ideas/open/420_rv64_gcc_torture_post_contract_umbrella.md` is an umbrella,
  not the next concrete packet.
- `ideas/open/442_pointer_value_memory_provenance_publication.md` remains
  parked because the no-external-caller/closed-world producer route was closed
  without an accepted source.
- Idea 422 is concrete, ordinary-C focused, and matches the umbrella handoff
  rule that BIR/prepared producer gaps must be handled before RV64 consumes
  missing facts.

## Suggested Next

Execute Step 1: audit current semantic producer failure evidence. Start from
the existing RV64 gcc_torture backend artifacts and identify rows whose first
owner is `backend object route requires semantic lir_to_bir lowering`.

## Watchouts

- Do not repair BIR-owned facts in RV64/MIR.
- Do not let F128 or `conversion.c` drive this route.
- Do not use expectation rewrites, unsupported downgrades, allowlists, or
  pass/fail accounting as progress.
- Keep canonical logs, review artifacts, implementation files, and tests
  untouched until an executor receives a bounded packet.

## Proof

Lifecycle validation:

```sh
git diff --check
python3 scripts/plan_review_state.py show
```

Result: passed.
