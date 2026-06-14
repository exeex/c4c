Status: Active
Source Idea Path: ideas/open/252_phase_f3_edge_publication_source_producer_blocker_map.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Select One Candidate Producer/Source Relation

# Current Packet

## Just Finished

Completed Step 2, "Select One Candidate Producer/Source Relation", as an
analysis-only selection for idea 252.

Selected narrow relation:

- The selected relation is same-edge CFG publication source-producer identity:
  the Route 5/BIR edge publication source record for one predecessor/successor
  edge and destination must identify the same source value and same-block
  producer as the prepared `PreparedEdgePublication` answer populated from
  `PreparedFunctionLookups::edge_publication_source_producers`.
- The route/BIR side is
  `bir::Route5CfgEdgePublicationRecord` plus MIR
  `BirCfgEdgePublicationSourceIdentity`, especially
  `source_value_kind`, `source_value_name`, `source_value_type`,
  `source_producer_kind`, `source_producer_instruction`,
  `source_producer_block_label_id`, and
  `source_producer_instruction_index`.
- The prepared side is the already-public prepared publication row:
  `PreparedEdgePublication::source_value_kind`, `source_value`,
  `source_producer_kind`, `source_producer_block_label`,
  `source_producer_instruction_index`, and the lookup source that produced
  those fields through `edge_publication_source_producers`.
- For `LoadLocal`, the selected relation includes the required companion
  source-memory identity: Route 5 must expose `MemorySource` with
  `source_memory_identity_available`, and the embedded Route 3 memory access
  must match the prepared `source_memory_access_status == available`,
  loaded result value, base identity, address space, volatility, offset, size,
  and alignment. For non-memory producer kinds, the relation stops at the
  producer/source identity and must not claim memory-source migration.

Agreement rule:

- The prepared publication and Route 5/BIR record must refer to the same
  predecessor label/id, successor label/id, destination value name/type, source
  value kind/name/type, source-producer kind after the existing kind mapping,
  source-producer predecessor block label/id, and source-producer instruction
  index.
- Named sources must have a non-null Route 5 `source_producer_instruction` and
  a BIR/MIR same-block producer identity whose produced value matches the
  prepared source value; immediate sources agree only through the immediate
  value/type and `Immediate` producer kind.
- `LoadLocal` agreement additionally requires the Route 3 memory-access fact to
  be available and to describe the same loaded value and memory source as the
  prepared source-memory fields. Missing, incomplete, absent, duplicate, or
  mismatched memory rows remain fail-closed cases for Step 5.
- A Route 5 status of `available` or `memory_source` is semantic evidence only
  when all of the fields above agree with the prepared publication. Status
  rows such as `no_match`, `no_source`, `missing_source_producer`, missing
  publication, and duplicate rows are diagnostic/fail-closed evidence, not
  authority transfer.

Excluded policy and compatibility surfaces:

- Publication availability is not the selected semantic relation by itself.
  Prepared `edge_publications` lookup status, missing-publication behavior, and
  edge-copy availability remain prepared compatibility authority.
- Move selection, register/home/storage policy, stack-source legality, carrier
  helper choice, scratch policy, branch/parallel-copy placement, and final
  source/destination operand selection are target policy, not producer/source
  identity.
- Instruction spelling and output formatting remain target owned, including
  x86 `mov` output and RISC-V `mv` / `lw` text, register names, offsets, and
  final emitted assembly.
- Helper/oracle names, prepared helper status strings, source-producer kind
  strings (`unknown`, `immediate`, `load_local`, `load_global`, `cast`,
  `binary`, `select_materialization`), source-memory status strings, Route 5
  diagnostic status names, fallback names, and prepared printer rows such as
  `source_producer=...`, `source_producer_block=...`, and
  `source_producer_inst=...` remain compatibility rows.
- AArch64/prepared helper consumers remain evidence that the public prepared
  helper surface must stay stable; they are not migration proof for x86 or
  riscv.

Out-of-plan boundaries:

- This plan will not broaden the selected relation into a generic BIR producer
  index, all same-block producer parity, generic publication parity, Route 6
  call-argument producer parity, AArch64 source-producer migration, or target
  output policy.
- Route 5 edge/source evidence alone is not readiness. Readiness would require
  Step 3 x86 evidence, Step 4 riscv evidence, and Step 5 duplicate/conflict/
  mismatch/missing/prepared-only fail-closed rows for this same selected
  relation.

## Suggested Next

Execute Step 3, "Prove or Block x86 Evidence", as an analysis-only packet for
the selected same-edge CFG publication source-producer identity relation.
Trace whether x86 directly or indirectly consumes the prepared
source-producer lookup answer or the Route 5/BIR source-producer identity for
the same edge, then classify x86 as proven, blocked, or explicitly
non-applicable with concrete file/function/test evidence.

## Watchouts

- Keep source idea 252 unchanged unless durable intent truly changes.
- Do not implement a BIR producer index or adapter in this blocker-map plan.
- Do not delete, privatize, wrap, rename, or bypass public prepared
  `edge_publication_source_producers` helpers.
- Do not infer readiness from Route 5 edge/source evidence alone; it is only
  the selected semantic fact to prove, block, or exclude per target.
- Treat x86 as having prepared edge-publication consumption but no observed
  source-producer consumer until Step 3 proves otherwise.
- Treat RISC-V Route 5/Route 3 agreement rows as diagnostic evidence until
  Step 4 rechecks them against this exact selected relation; prepared output
  and fallback remain authoritative.
- Preserve helper/oracle names, compatibility strings, fallback names,
  publication/output rows, and target-policy-sensitive behavior.
- Treat testcase-shaped shortcuts, expectation weakening, helper/status/output
  renames, and classification-only maps claimed as implementation progress as
  route drift.

## Proof

Analysis-only packet. No build or test proof required by supervisor.
Local validation: `git diff --check -- todo.md`.
