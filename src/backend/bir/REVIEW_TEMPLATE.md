# Architecture Review Template

Use this checklist for every pass, analysis, planner, and MIR stage. Reviews
must compare both the preceding and following stage; reviewing one document in
isolation is insufficient.

## Contract review

- Is the input stage token exact?
- Are all consumed facts owned by the input or a named revision-bound analysis?
- Is the output authoritative, or merely diagnostic?
- Are mutation, invalidation, and failure behavior explicit?
- Can the next stage consume the output without consulting legacy state?
- Does the stage preserve stable IDs where required?
- Is verification performed at the correct boundary?

## Ordering review

- What invariant is established by the preceding stage?
- Why can this stage not run earlier?
- Which later stages depend on its postconditions?
- Is re-entry forbidden, idempotent, or fixed-point bounded?
- Which analyses are required, preserved, or invalidated?

## Legacy completeness review

- Which exact files and symbols under `src/backend/legacy/` are covered?
- Are nearby same-feature paths included, rather than only known testcases?
- Are target-independent semantics separated from target realization?
- Are error paths, diagnostics, object data, inline assembly, variadics,
  atomics, runtime helpers, and uncommon integer/float widths accounted for?
- What legacy reader becomes unnecessary once this contract is implemented?

## Rejection conditions

- The design stores an analysis result as duplicate persistent authority.
- It uses names, pointers, indices, or rendered text as semantic identity.
- It writes target allocation/frame/ABI decisions into canonical BIR.
- It requires an unspecified later pass to repair malformed output.
- Its main proof covers only a named testcase or rewrites expectations.
