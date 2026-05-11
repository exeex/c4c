Status: Active
Source Idea Path: ideas/open/163_backend_semantic_bir_route_alignment.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory and Classify the 31 Baseline Failures

# Current Packet

## Just Finished

Lifecycle activation created `plan.md` and initialized this execution-state file for Step 1.

## Suggested Next

Start Step 1 by extracting the 31 backend route failures from `test_baseline.log`, grouping them by failure family, and recording the first narrow implementation target here before editing implementation files or expected snippets.

## Watchouts

- Do not mark supported route cases unsupported or weaken semantic BIR expectations.
- Do not update snippets until the actual BIR is classified as semantically correct.
- A semantic BIR observation that receives LLVM IR should be fixed at the route or harness layer.
- Do not add named-test shortcuts; prove repairs across nearby same-feature cases.

## Proof

Lifecycle-only activation; no build or test proof required yet.
