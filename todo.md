Status: Active
Source Idea Path: ideas/open/248_phase_f2_x86_riscv_prepared_boundary_completion_criteria_audit.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Define Prepared Public API Exit Criteria

# Current Packet

## Just Finished

Step 4 from `plan.md` defined prepared public API exit criteria for every
public lookup group, `PreparedBirModule` public/control field, compatibility
surface, target-policy row, and private pass-context candidate named by the
active plan. No implementation, tests, baselines, source idea, or `plan.md`
changes were made.

### Step 4 Prepared Public API Exit Criteria

| Public API or surface | Exit path | Safe transition condition | Stable compatibility and blocker notes |
| --- | --- | --- | --- |
| `PreparedFunctionLookups::call_plans`, `PreparedBirModule::call_plans`, and `find_prepared_call_plans` | Split target-policy plus BIR semantic index, retained compatibility until migration; blocked today | Safe when Route 6 or another route/BIR call-use source index is the semantic authority for all x86 call-plan consumers, riscv has either matching consumer evidence or explicit fail-closed non-applicability, and public prepared lookup/module answers are only compatibility mirrors. | Preserve `ConsumedPlans`, direct-call wrapper output, x86 route-debug names, prepared fallback names, helper/oracle statuses, and unsupported fail-closed rows. ABI call sequence, argument/result layout, wrapper instruction text, stack/register policy, and helper selection remain target policy. |
| Route 6 scalar call-use/source identity adjacent to call plans | BIR semantic index behind retained prepared compatibility | Safe when the narrow accepted scalar `i32` evidence is extended to direct-call and wrapper cases, route/prepared mismatch is fail-closed, and both targets either consume or explicitly exclude the fact family with evidence. | Public prepared call-plan APIs cannot exit while Route 6 evidence is x86-narrow and riscv coverage is absent. Compatibility output must keep existing status/debug/fallback strings. |
| Direct-call wrapper output and `ConsumedPlans` rows | Retained compatibility adapter | Safe only to keep as a public compatibility/oracle surface; demotion is safe after semantic call identity comes from route/BIR facts and wrapper baselines prove unchanged output. | Wrapper assembly/output rows, `ConsumedPlans`, route-debug wrapper names, prepared fallback names, and public expected strings must remain stable unless a separate target-policy change is approved. |
| `PreparedFunctionLookups::memory_accesses` and public indexed memory lookup helpers | Split BIR memory/source semantic index plus retained compatibility; blocked today | Safe when x86 has direct or adapter consumption of route/BIR memory/source identity, riscv Route 3/source-memory rows prove the same semantic fact drives behavior, and prepared lookup/status answers agree or fail closed on mismatch. | Preserve prepared source-memory statuses, helper/oracle names, fallback names, unsupported rows, and policy-sensitive addressing/storage output. Addressing modes, frame/storage placement, load/store instruction choice, and register materialization stay target-owned. |
| Route 3 memory/source identity facts | BIR semantic index behind retained prepared status/fallback compatibility | Safe when x86 evidence is no longer missing or explicitly proven non-applicable, riscv status rows are route/BIR-driven rather than prepared-only, and missing/invalid/duplicate/mismatch cases fail closed. | Public `memory_accesses` cannot exit from route evidence alone. Prepared source-memory status strings and fallback rows remain observable compatibility authority. |
| `PreparedFunctionLookups::edge_publications` and public edge publication lookup helpers | Split Route 4/5 BIR semantic index plus target-owned emission policy; retained compatibility until migration; blocked today | Safe when x86 dispatch/status/module output and riscv emission/status consume or validate the same Route 4/5 fact through adapters, prepared lookup answers only mirror that fact, and mismatch/duplicate/unsupported rows fail closed. | Preserve `EdgePublicationMoveIntentStatus`, x86 dispatch/status names, riscv prepared publication statuses, fallback publication rows, exact riscv instruction text, helper/oracle names, and module output rows. Move selection, registers, carriers/helpers, layout, and instruction text remain target policy. |
| Route 4/5 edge-publication identity facts | BIR semantic index behind retained prepared publication compatibility | Safe when x86 and riscv both have route/BIR agreement evidence for edge/source identity without changing public status strings or instruction output. | Public `edge_publications` remains authority until status/emission consumers no longer depend on prepared lookup data as the answer. |
| x86 and riscv edge-publication prepared dispatch/status surfaces | Retained compatibility adapter | Safe only as compatibility until semantic edge facts move behind adapters; transition is safe when public status rows are proven unchanged and route/BIR mismatch fails closed. | Keep `EdgePublicationMoveIntentStatus`, prepared publication status strings, x86 dispatch behavior, riscv exact instruction output, fallback names, and helper/oracle rows stable. |
| `PreparedFunctionLookups::edge_publication_source_producers` and source-producer lookup helpers | Blocked public prepared authority with possible future BIR producer index | No exit path is safe until x86/riscv direct consumption or explicit non-applicability is proven, AArch64/prepared helper consumers are migrated or retained by a compatibility adapter, and duplicate/conflict/mismatch producer rows fail closed. | Existing AArch64 and prepared helper/oracle compatibility use keeps this public. Preserve source-producer helper names, fallback names, and publication/output rows. |
| Edge-publication source-producer identity fact | Blocked BIR semantic index candidate | Safe only after the producer/source relation is defined independently of prepared lookup storage and x86/riscv parity or exclusion evidence exists. | Do not infer exit readiness from Route 5 edge/source evidence alone; current matrix lacks direct x86/riscv source-producer consumers. |
| `PreparedBirModule::module` | Retained compatibility wrapper around BIR module; deletion/demotion after consumer migration | Safe when targets/tests use a route/BIR-facing module interface instead of mutating or reading BIR through the prepared aggregate, and the public prepared aggregate can be kept only as a compatibility wrapper or deleted after all consumers migrate. | Preserve prepared aggregate behavior, printer/module output rows, and fail-closed behavior for missing/invalid/duplicate module/function/block facts. Target traversal/lowering policy remains outside BIR semantics. |
| Underlying BIR module/function/block structure | BIR semantic authority | Safe when exposed through a BIR-owned interface rather than public prepared aggregate fields, with prepared/BIR mismatch and unsupported BIR shapes covered. | This is not itself a prepared public API exit blocker; the blocker is the public aggregate wrapper and existing target/test access pattern. |
| `PreparedBirModule::names` | Split BIR semantic name index from retained debug/formatting compatibility; blocked today | Safe when semantic name-to-id lookup has a route/BIR-owned replacement consumed by relevant targets, and diagnostic/rendering/formatting users are separated behind stable compatibility or target-policy surfaces. | Preserve x86 route-debug names, prepared printer formatting, AArch64 formatting/lookup output, fallback name rendering, and duplicate/conflict name failure rows. Debug text, formatting style, route-debug output, and target labels are not BIR ownership. |
| Name-to-id/name-resolution semantic index | BIR semantic index candidate, blocked | Safe when the semantic index is independently published and x86/riscv consumer or non-applicability evidence exists. | Public `names` remains authority until semantic lookup and rendering/debug compatibility are split. |
| Name rendering, debug text, and formatting use of `names` | Retained compatibility adapter or target-policy surface | Safe only as stable compatibility/target output; not a deletion signal for `names` until semantic consumers migrate. | Existing route-debug text, prepared printer text, formatting output, and fallback rendering names must remain stable. |
| `PreparedBirModule::control_flow` | Split BIR CFG/dominance/block index from target branch/layout policy; blocked today | Safe when x86 and any relevant riscv/AArch64 control-flow consumers use route/BIR CFG facts or documented adapters, prepared helpers are compatibility mirrors, and missing/invalid/duplicate/mismatch CFG rows fail closed. | Preserve prepared block helper names, dominance/successor behavior, route-debug rows, fallback helper paths, and unsupported CFG cases. Branch lowering, layout, labels, and instruction spelling remain target policy. |
| CFG/dominance/block-index helper facts | BIR semantic index candidate, blocked | Safe when helper facts are published as BIR/route-owned facts and consumers no longer require public prepared helper authority for semantic answers. | Current prepared helper/oracle compatibility remains public until route/BIR ownership is proven across consumers. |
| `PreparedBirModule::store_source_publications` | Split BIR store-source semantic records plus retained source-memory/status compatibility; blocked today | Safe when x86/riscv source-publication behavior is either directly consumed from route/BIR records or explicitly excluded with fail-closed proof, and prepared printer/status/fallback rows only mirror the semantic record. | Preserve prepared printer rows, source-memory status strings, fallback names, helper/oracle statuses, and addressing/storage-sensitive output. Current evidence is indirect, so no demotion/deletion is safe. |
| Store-source publication semantic record | BIR semantic index candidate, blocked | Safe when the source/publication relation is independent of the prepared module field and both target surfaces are covered. | Public field and prepared source-memory/status compatibility stay authoritative until consumers are mapped. |
| Prepared lookup helper/oracle status names | Retained compatibility adapter | Safe only to retain. Any demotion behind private adapters requires exact status-name preservation and tests showing semantic decisions moved without renaming or weakening oracle contracts. | Compatibility strings, helper/oracle statuses, missing/incomplete/unsupported/fallback names, and aggregate stack/source authority rows must remain stable. These names cannot be hidden or changed as proof of ownership transfer. |
| Fallback behavior and unsupported fail-closed behavior | Retained compatibility requirement for every exit path | Safe when each candidate fact family proves missing, invalid, duplicate/conflict, mismatch, unsupported, prepared-only, fallback, and policy-sensitive cases fail closed or agree with route/BIR facts. | Fallback names and unsupported behavior are public compatibility authority until replacement facts prove matching behavior. Fallback may observe target policy but cannot move policy into BIR. |
| x86 route-debug output | Retained compatibility adapter | Safe when route-debug semantic claims are backed by route/BIR facts and output remains stable; riscv parity is required only if riscv exposes an analogous debug surface. | Preserve route-debug names, printed identity/status strings, fallback debug output, and target-specific rendered details. Debug formatting is compatibility/target policy, not BIR ownership. |
| Wrapper assembly/output baselines | Retained compatibility adapter plus target-owned policy product | Safe only after semantic claims inside wrapper rows are route/BIR-backed and wrapper text/bytes remain unchanged, or a separate target-policy change explicitly approves output changes. | Preserve wrapper assembly/output baselines, direct-call wrapper rows, fallback wrapper names, ABI/register/stack/layout behavior, helper choice, and instruction spelling. |
| ABI, layout, register, stack, storage, addressing, carrier/helper, formatting, emission, instruction spelling, and wrapper policy | Target-owned policy product; not a BIR/prepared semantic exit target | Safe condition is negative: future exits must prove these stay in target lowering/emission or stable compatibility baselines and are not reclassified as BIR semantic facts. | Prepared may carry compatibility/fallback observations of these policies. Any row whose exit requires moving target policy into BIR is blocked. |
| `PreparedBirModule::route` | Private prepared pass context candidate | Safe when prepared printer/status/debug text is covered by a private adapter or stable text contract and no x86/riscv target-facing public consumer is found. | Preserve printer route output and any status/debug strings. Demotion should not redefine this as BIR semantic fact or target policy. |
| `PreparedBirModule::invariants` | Private prepared pass context candidate | Safe when printer/status compatibility is retained and no public target-facing consumer is found. | Preserve prepared printer invariant output and fail-closed handling for invalid/mismatched invariant state. |
| `PreparedBirModule::completed_phases` | Private prepared pass context candidate | Safe when completed-phase printer/status output is preserved through a private adapter and no target-facing public consumer depends on the field. | Preserve completed-phase text/status rows. Demotion is not safe if public diagnostics require direct field access. |
| `PreparedBirModule::notes` | Private prepared pass context candidate | Safe when notes printer/status output is preserved through a private adapter and no target-facing public consumer depends on the field. | Preserve notes text/status rows and fallback behavior for absent notes. |
| `PreparedBirModule::liveness` | Blocked private pass-context/planning authority | No exit path is safe until prealloc/regalloc/helper-planning consumers are mapped, replacement ownership is defined, and x86/riscv relevance or non-applicability is proven with fail-closed behavior. | Keep liveness separate from simple metadata. Preserve helper-planning compatibility and any liveness-driven fallback/status rows. Target register/storage policy remains target-owned. |

### Exit Labels

- Retained compatibility means the public API remains observable authority for
  strings, statuses, fallback behavior, debug output, wrapper output, or helper
  oracle rows even if a route/BIR fact moves underneath it.
- Private pass context candidate means public target-facing authority was not
  observed, but printer/status compatibility must be preserved before any
  demotion.
- Split target-policy plus BIR semantic index means the semantic identity can
  become route/BIR-owned only after target policy and stable compatibility
  output are explicitly separated.
- Deletion after consumer migration requires all consumers to use the new
  semantic or compatibility surface first; no current row is deletion-ready.
- Blocked means current evidence lacks x86/riscv parity, consumer migration,
  compatibility separation, or fail-closed coverage.

## Suggested Next

Execute Step 5 from `plan.md`: generate bounded follow-up ideas for the rows
whose exit criteria are concrete enough, and record the draft 155 disposition
without creating a broad prepared-retirement implementation idea.

## Watchouts

- No public lookup group or `PreparedBirModule` field is deletion-ready from
  the current matrix. The only near-term demotion candidates are `route`,
  `invariants`, `completed_phases`, and `notes`, and even those need
  printer/status compatibility coverage first.
- `liveness` is still active prepared planning authority, not simple metadata.
- Compatibility strings, helper/oracle statuses, route-debug names, fallback
  names, wrapper-output rows, and exact target output must remain stable.
- Target policy boundaries remain hard blockers: ABI, layout, registers, stack,
  storage, addressing, carriers/helpers, formatting, emission, instruction
  spelling, and wrapper behavior stay outside BIR ownership.

## Proof

No build/test proof required by the delegated packet. No new spot-check
commands were run for Step 4; the exit criteria use the committed Step 2 matrix
and Step 3 readiness criteria already recorded in canonical `todo.md` history.

No `test_after.log` was produced because the packet explicitly requested no
build/test proof and no root-level log files.
