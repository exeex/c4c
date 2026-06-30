Status: Active
Source Idea Path: ideas/open/468_carrier_alias_identity_publication_api.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Carrier Alias Identity Callers

# Current Packet

## Just Finished

Completed Step 1 audit for idea 468. The caller/API matrix shows that carrier
alias identity must be published into one shared prepared-module name table
before const consumers run.

Caller matrix:

| Surface | Module access | Shared identity need | Classification |
| --- | --- | --- | --- |
| Planner input (`publication_plans.hpp:729`) | `const PreparedNameTables*` | Needs mutation if it publishes alias `ValueNameId`s. | Current API is read-only; hidden mutation is rejected. |
| Planner lookup (`publication_plans.cpp:2794`, `:3090`) | Existing name lookup only | Needs alias names to exist. | Exact current `unsupported_carrier_alias` rejection point. |
| Evidence collector (`publication_plans.cpp:3199`) | `const PreparedBirModule&` | Needs stable identity for available records. | Not a safe mutation site. |
| Available-authority collector (`publication_plans.cpp:3184`) | `const PreparedBirModule&` | Feeds RV64 consumers. | Must read already-published canonical identity. |
| Prepared dump (`prepared_printer.hpp:11`, `select_chains.cpp:240`) | `const PreparedBirModule&` | Needs stable display names. | Read-only; must not be the publisher. |
| RV64 function route (`object_emission.cpp:8979`, `:9009`) | `const PreparedBirModule&` | Needs available authority records. | Not a safe mutation site. |
| RV64 alias-select suppression (`object_emission.cpp:7420`) | Original `prepared.names` lookup | Needs alias result name in the original table. | Proves scratch-copy publication is insufficient. |
| Prealloc contract-plan phase (`prealloc.cpp:30`, `:42`) | Mutable `PreparedBirModule&` | Can publish before const handoff. | First viable API boundary. |
| Backend RV64 handoff (`backend.cpp:1566`, `:1570`, `:212`) | Prepared module is produced then passed const | Needs publication before handoff. | Confirms pre-consumer publication boundary. |

Selected first viable boundary: define a producer-owned mutable publication
stage in prealloc, before prepared dump/RV64 const consumers. The stage should
publish carrier-alias `ValueNameId`s into canonical `PreparedNameTables` after
semantic candidate checks pass. It must not use `const_cast`, scratch-copy
authority, raw `.phi.sel` inference, or RV64 lowering.

Artifact:
`build/agent_state/468_step1_carrier_alias_identity_callers/audit.md`.

## Suggested Next

Execute Step 2: define the shared identity publication contract. Recommended
target is a mutable pre-consumer publication stage such as
`populate_select_carrier_alias_identity(PreparedBirModule&)`, with focused
positive and negative contract cases for Step 3.

Step 2 should define:

- required semantic facts for publishing a synthesized carrier-alias name;
- whether source-use closure is enforced in the publication stage or rechecked
  by the authority collector;
- how prepared dump and RV64 const consumers observe the canonical name table;
- rejected shapes for wrong edge/source/final result, duplicate candidates,
  raw-name-only aliases, extra source uses, and scratch-copy-only publication.

Expected Step 2 proof:

```sh
git diff --check
```

## Watchouts

- Do not edit implementation in Step 2 unless explicitly delegated.
- Do not reintroduce `const_cast` or hidden mutation.
- Do not claim scratch-copy-only publication is sufficient for original-module
  consumers.
- Do not make RV64 lowering changes in this identity/API idea.
- Do not infer aliases from `%*.phi.sel*` spelling, testcase names, dump order,
  value ids, block labels, or raw shape alone.
- Do not modify `test_baseline.new.log`, `test_baseline.log`,
  `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 1 proof:

```sh
git diff --check
```

Result: passed.
