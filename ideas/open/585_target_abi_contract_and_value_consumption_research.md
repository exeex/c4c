# Target ABI Contract And Value Consumption Research

Status: Open
Type: Research and architecture documentation
Parent: `ideas/open/584_rv64_stack_destination_move_bundle_authority_contract.md`
Owning Layer: Target ABI policy, BIR call ABI facts, prepared/prealloc value
consumption contracts, and RV64/AArch64 backend consumption boundaries

## Goal

Produce concrete research documents under
`docs/target_abi_contract_research/` that explains whether the current
target-profile, BIR ABI, prepared/prealloc, and backend contracts can carry the
calling-convention and value-consumption information needed by both AArch64
and RV64.

Each research question must have its own Markdown file. The documents should
answer the open architecture questions before any
implementation or contract rewrite is proposed.

## Why This Exists

The current route deliberately makes target information available early:
target triples become `TargetProfile`, BIR lowering uses that profile to
produce call ABI facts, prepared/prealloc maps those facts into register
placements and move plans, and each target backend consumes the prepared
surface.

That chain is directionally correct and is required if x86, AArch64, and RV64
are to share a register allocator. The unresolved question is whether the
existing contract is rich enough, and consumed consistently enough, to express
the hard cases now exposed by recent closed ideas:

- prepared value reuse versus producer rematerialization
- call-boundary preservation and republication
- caller/callee-saved pool authority
- ABI argument/result register identity
- select/carrier/no-home publication
- stack-destination move-bundle authority

Recent tail work cleared several straightforward RV64 object-emission gaps.
The remaining failures are more architectural: they tend to appear only after
BIR and prepared facts exist, when prealloc or backend consumers must decide
whether a value home is fresh, whether a producer should be rematerialized,
and whether a move/publication has enough authority to be trusted.

## Research Questions And Required Answer Files

There are six research questions. The delivery must contain exactly six
question-answer Markdown files, one for each question, plus one `index.md`.
Each answer file must answer only its assigned question and may link to the
other answer files for supporting context.

1. `01_how_target_information_enters_the_pipeline.md`

   Question: How does target information currently enter the pipeline, from
   `TargetProfile` through BIR call ABI metadata, prepared register placements,
   and final target emission?

   Required answer shape:
   - trace the actual code path from target triple/profile creation to BIR
     lowering context
   - trace where BIR call arg/result ABI facts are produced
   - trace where prepared/prealloc maps abstract ABI facts to target register
     placements
   - trace where RV64 and AArch64 backends consume the prepared surface
   - state whether the current direction of information flow is intentional
     and healthy

2. `02_are_current_contract_fields_sufficient.md`

   Question: Are `CallArgAbiInfo`, `CallResultAbiInfo`,
   `PreparedRegisterPlacement`, `PreparedTargetRegisterIdentity`, move
   bundles, call plans, and preservation plans sufficient to carry the ABI
   facts needed by both AArch64 and RV64?

   Required answer shape:
   - table each named contract surface and the facts it currently carries
   - identify which facts are semantic ABI facts, allocation-policy facts, and
     physical target facts
   - compare AArch64 and RV64 requirements explicitly
   - answer `sufficient`, `insufficient`, or `sufficient with named
     limitations`
   - list the exact limitations or missing fields if the answer is not simply
     sufficient

3. `03_where_target_facts_are_split.md`

   Question: Where are target facts currently split between `target_profile`,
   BIR lowering, prepared/prealloc target register profiles, and backend
   emission, and which splits are healthy versus accidental?

   Required answer shape:
   - list each code surface that owns part of the target ABI/register contract
   - state what each surface owns today
   - mark each split as healthy, suspicious, or accidental
   - explain whether any split blocks shared x86/AArch64/RV64 regalloc goals
   - identify the smallest surface that could become a coherent target ABI
     policy if consolidation is recommended

4. `04_current_prepared_value_consumption_model.md`

   Question: What is the current decision model for consuming a prepared
   value: reuse an existing home, rematerialize from the producer,
   copy/publish into a destination, or fail closed?

   Required answer shape:
   - describe the current reuse/rematerialize/copy/fail decision order using
     concrete code paths
   - identify which producer kinds are currently rematerialized explicitly
   - identify which consumer contexts choose reuse or preservation fallback
   - identify where fail-closed diagnostics protect unknown authority
   - state whether the current decision model is centralized or distributed

5. `05_prior_preservation_freshness_and_stale_home_risk.md`

   Question: Where does `PriorPreservation` or equivalent preservation
   fallback have enough authority, and where can it hide stale-home or
   missing-producer bugs?

   Required answer shape:
   - define what freshness means in the current prepared/prealloc contract
   - explain when prior preservation is valid today
   - explain when producer rematerialization should outrank prior preservation
   - cite concrete stale-home or missing-producer risk evidence from code or
     closed ideas
   - list the minimum facts needed to make preservation authority explicit

6. `06_closed_idea_tails_and_followup_questions.md`

   Question: Which recent closed ideas mention tails related to target ABI
   contracts, value home freshness, publication, preservation,
   rematerialization, or move-bundle authority, and what concrete follow-up
   questions do they leave?

   Required answer shape:
   - table every relevant closed idea reviewed
   - quote or summarize the closure tail being used as evidence
   - classify each tail as target ABI policy, value freshness, publication,
     preservation/rematerialization, move-bundle authority, or unrelated
   - identify whether the tail is already covered by an open idea
   - list the remaining concrete follow-up questions that still need
     discussion after the research

## Required Documentation Output

Create the research documents in:

```text
docs/target_abi_contract_research/
```

Required files:

- `docs/target_abi_contract_research/index.md`
- `docs/target_abi_contract_research/01_how_target_information_enters_the_pipeline.md`
- `docs/target_abi_contract_research/02_are_current_contract_fields_sufficient.md`
- `docs/target_abi_contract_research/03_where_target_facts_are_split.md`
- `docs/target_abi_contract_research/04_current_prepared_value_consumption_model.md`
- `docs/target_abi_contract_research/05_prior_preservation_freshness_and_stale_home_risk.md`
- `docs/target_abi_contract_research/06_closed_idea_tails_and_followup_questions.md`

`index.md` must link to all six answer files and summarize the overall result.
It must not replace any required answer file.

## In Scope

- Read and cite current target-profile, BIR lowering, prepared/prealloc, and
  RV64/AArch64 backend call/value consumption surfaces.
- Review relevant closed ideas for evidence of stale home reuse, insufficient
  producer rematerialization, publication holes, preservation ambiguity, and
  move-bundle authority gaps.
- Compare AArch64 and RV64 consumption paths where both exist, especially call
  ABI placement, caller/callee-saved pools, call-boundary effects, and
  prepared value publication.
- Document current behavior and risks without changing implementation.
- Identify which architectural questions need human discussion before a
  source idea for implementation should be written.

## Out Of Scope

- Implementation changes in BIR, prepared/prealloc, RV64, AArch64, x86, or
  tests.
- Activating the idea into `plan.md` unless explicitly requested later.
- Rewriting closed ideas or changing lifecycle history.
- Turning the research directly into a broad refactor plan without first
  producing the six required answer files.
- Treating one target-only workaround as proof that the shared contract is
  sufficient.

## Acceptance Criteria

- `docs/target_abi_contract_research/` contains one `index.md` plus exactly
  one `.md` answer file for each numbered question in
  `## Research Questions And Required Answer Files`.
- The six answer filenames match the six required filenames exactly.
- Each answer file answers its assigned question directly and follows its
  `Required answer shape`.
- The number of answer files equals the number of numbered research questions;
  do not merge two questions into one file and do not split one question into
  multiple primary answer files.
- The documents cite concrete code surfaces and relevant closed ideas rather
  than relying on generic architecture claims.
- `02_are_current_contract_fields_sufficient.md` states whether the current
  target-profile and prepared/prealloc contracts are sufficient,
  insufficient, or sufficient only with named limitations for AArch64/RV64
  dual-platform support.
- `06_closed_idea_tails_and_followup_questions.md` classifies every remaining
  tail and lists concrete follow-up questions.
- `index.md` includes a final recommendation table classifying each recommended
  follow-up as documentation, narrow implementation idea, or
  discussion-required architecture work.
- No implementation files, test expectations, unsupported markers, or runtime
  comparison behavior are changed.

## Reviewer Reject Signals

- Reject a submission whose answer-file count does not equal the numbered
  research-question count.
- Reject a submission that collapses multiple numbered questions into one
  large report or splits one numbered question across multiple primary answer
  files.
- Reject an answer file that does not follow the required answer shape for its
  numbered question.
- Reject an answer file that only restates the desired pipeline without
  tracing the actual code path from `TargetProfile` through BIR ABI facts and
  prepared target register placement where that trace is required.
- Reject claims that the contract is sufficient or insufficient without
  evidence from both code surfaces and closed idea notes.
- Reject testcase-shaped analysis that only discusses one representative, one
  function, or one backend path while ignoring the AArch64/RV64 dual-platform
  question.
- Reject broad implementation recommendations that are not separated from
  documentation findings and discussion-required architecture decisions.
- Reject changes to implementation, expectations, unsupported markers,
  allowlists, or lifecycle history under this research idea.
- Reject answer files that fail to identify the remaining concrete tails from
  relevant `ideas/closed/` files.
