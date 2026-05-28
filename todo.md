Status: Active
Source Idea Path: ideas/open/59_aarch64_dispatch_family_contraction_audit.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Classify Each Dispatch File Or Helper Group

# Current Packet

## Just Finished

Completed `plan.md` Step 2 classification for idea 59. This is classification
only; no implementation capability progress is claimed.

Classification vocabulary:

- `fold-back`: standalone dispatch-family owner can disappear by merging into
  an existing precise AArch64 owner.
- `move-forward`: semantic authority belongs in shared BIR/prealloc/prepare
  rather than an AArch64 dispatch helper.
- `keep-local`: AArch64 target emission, scratch/clobber handling,
  diagnostics, instruction spelling, or final sequencing justifies local
  ownership.
- `split-owner`: the current file mixes ownership classes and should be split
  into smaller target/shared follow-up work.
- `needs-more-evidence`: a narrow audit/proof is required before a durable
  contraction recommendation.

Durable classification table:

| File or helper group | Ownership class | Evidence note | Expected disposition | Proof gap |
| --- | --- | --- | --- | --- |
| `dispatch.hpp` / `dispatch.cpp` public block entry points | `keep-local` | AST shows only `make_block_lowering_context` and `dispatch_prepared_block` as public entry points; `traversal.cpp` calls them as the AArch64 prepared-block route. The code sequences block-entry copies, family dispatch, terminator handling, diagnostics, and post-block edge-copy publication. | Stay as the local block traversal/routing owner, possibly after narrower helpers leave the dispatch family. | None for classification. |
| `dispatch.cpp` routing and diagnostics group | `keep-local` | `classify_instruction`, unsupported-instruction/terminator diagnostics, BIR block lookup, block-context lookup, and `make_bir_machine_instruction` are target route glue and diagnostic/reporting surfaces. | Keep local with `dispatch.cpp`; do not move to shared authority. | None. |
| `dispatch.cpp` before-return publication bookkeeping and branch-fusion hook wiring | `split-owner` | `record_before_return_publication` and `make_dispatch_branch_fusion_hooks` wire publication/value-materialization/producers into dispatch. The decisions they call into are owned elsewhere, but sequencing is local. | Keep the sequencing in dispatch; let follow-up splits move producer/publication helper authority out of `dispatch_*` files first. | None; disposition depends on producer/publication follow-ups. |
| `dispatch.cpp` address-materialization and store/scalar retry routing | `keep-local` | `lower_store_local_with_address_materialization` and `lower_scalar_with_address_materialization` route prepared address-materialization records into AArch64 instruction emission and diagnostics without being the semantic owner of address facts. | Stay local unless later folded into `memory.cpp`/`alu.cpp` by a mechanical target-owner split. | No semantic proof gap; layout destination can be decided in Step 3. |
| `dispatch_edge_copies.hpp` / `dispatch_edge_copies.cpp` file owner | `split-owner` | Public surface mixes true edge-copy emission with `emit_select_chain_value_to_register` callers from `alu.cpp`/value materialization and `materialize_direct_global_select_chain_call_argument` from `calls.cpp`. Ideas 47/58 closed edge publication and clobber repairs here, so remaining file is not one ownership class. | Preserve edge-copy lowering locally; split select-chain/direct-global call-argument helpers into precise owners or shared authority follow-ups. | None for file-level split classification. |
| Edge prepared-consumer group | `keep-local` | Prepared edge-publication source matching, prepared source producer context recovery, prepared memory access matching, source register/home extraction, and va-list carrier emission consume `PreparedEdgePublication`, `PreparedEdgePublicationSourceProducer`, `PreparedMemoryAccess`, and value homes after idea 47. | Stay in the AArch64 edge-copy owner as target emission over prepared facts. | None. |
| Edge local-fallback producer group | `move-forward` | `find_edge_named_producer` still calls same-block producer lookup across successor/edge/predecessor contexts when prepared publication is absent; idea 47 explicitly rejected predecessor-depth scans as durable authority. | Follow-up should either fail closed behind prepared facts or add/consume a shared edge dependency query. | Need a narrow proof of current null-publication callers before deleting fallback behavior. |
| Edge hazard/scratch group | `keep-local` | Recursive `edge_value_publication_may_read_register_index`, scratch/result register choice, and final move/load/store ordering are target-local clobber avoidance, and idea 47/58 closure notes preserve clobber behavior as local. | Stay local in edge-copy emission. | None. |
| Edge select-chain materialization group | `split-owner` | AST/caller evidence shows `emit_select_chain_value_to_register` is public to edge copies, `alu.cpp`, and value materialization, while it also reaches producer lookup and direct-global dependency facts. | Split into a precise select/materialization owner; semantic dependency discovery should consume shared prepared select-chain authority. | Need follow-up proof that non-edge callers are covered by existing prepared dependencies. |
| Direct-global select-chain call-argument group | `move-forward` | `materialize_direct_global_select_chain_call_argument` is called by `calls.cpp`; idea 47 called this a possible `PreparedCallPlan` or shared select-chain dependency responsibility. | Move semantic direct-global/select-chain argument authority to call plan or shared prepared query; keep final AArch64 emission in calls/select materialization. | Need narrow proof of current prepared call-argument coverage. |
| `dispatch_lookup.hpp` / `dispatch_lookup.cpp` prepared lookup bridges | `fold-back` | `prepared_named_value_id`, `prepared_value_id`, and `find_value_home` are thin wrappers over `PreparedValueHomeLookups`/indexed prepared homes and have no AArch64-specific emission policy. | Fold into shared prepared lookup helpers or inline into precise target owners; this file should shrink or disappear once raw scan helpers move. | None. |
| `dispatch_lookup.cpp` emitted scalar and prepared result-register helpers | `split-owner` | `emitted_scalar_value_available` consults local `BlockScalarLoweringState`; `make_named_prepared_result_register` converts prepared home facts to AArch64 register operands. | Move state-local availability to scalar/publication owner and register conversion to publication/register helper owner. | None. |
| `dispatch_lookup.cpp` same-block scalar/load-local availability helpers | `move-forward` | `find_same_block_scalar_producer` and `has_same_block_load_local_producer` scan BIR same-block producers; Step 1 flagged raw producer scans as suspect and prior closures reject new same-block authority. | Replace call sites with shared `mir::query` only where it is a convenience, or shared prepared scalar/source facts where semantic authority is needed. | Need caller-by-caller audit before removal. |
| `dispatch_producers.hpp` / `dispatch_producers.cpp` file owner | `split-owner` | AST shows one file combines `mir::query` wrappers, prepared select bridge, direct-global select-chain dependency traversal, global symbol helpers, and join parallel-copy cache helpers. | Split into shared/prealloc authority follow-ups and target-owner moves; standalone `dispatch_producers.*` should disappear. | None for file-level split classification. |
| Producer wrappers over `mir::query` | `fold-back` | `find_same_block_binary_producer`, `find_same_block_named_producer`, and `evaluate_same_block_integer_constant` mostly delegate to `mir::query`; direct callers include comparison, FP materialization, edge copies, and dispatch hooks. | Fold thin wrappers into callers or `mir::query` consumers; do not create new target authority. | Need call-site audit to avoid changing semantics while removing wrappers. |
| Prepared same-block select bridge | `keep-local` | `find_prepared_same_block_select_producer` first consumes prepared source-producer facts, then falls back to same-block select discovery for convenience. | Keep only as a target-local prepared consumer if fallback is proven non-authoritative; otherwise split the fallback into shared authority. | Need fallback-path proof. |
| Direct-global select-chain dependency traversal | `move-forward` | `prepared_select_chain_contains_direct_global_load` mirrors shared `prealloc/publication_plans.cpp` logic and feeds select/call materialization. | Move to or consume the shared prepared select-chain dependency authority. | Need proof that shared prepared result has equivalent coverage. |
| Load-global target/symbol helpers | `split-owner` | `find_load_global_target` and `load_global_symbol_label` are used by global-load materialization, while GOT/direct policy appears in value materialization. | Move to `globals.cpp` or a global materialization helper; keep only AArch64 symbol spelling local. | None. |
| Current-block join parallel-copy cache helpers | `move-forward` | `build_current_block_join_parallel_copy_cache` reconstructs incoming expression/source relationships from prepared homes and move bundles; ideas 47/48 already moved similar move/publication authority into shared/prepared helpers. | Add/consume shared prepared join-copy query, then remove local cache authority. | Need narrow proof for current dispatch caller and nearby join-copy cases. |
| `dispatch_publication.hpp` / `dispatch_publication.cpp` file owner | `split-owner` | Public helpers span register/address spelling, prepared publication consumption, local-slot/address materialization, branch/fused-compare publication, retargeting, fixed-formal stores, and clobber checks. Idea 48 closed semantic fallback repair but the file remains a mixed owner. | Keep target publication emission local, but split spelling utilities and any remaining shared semantic queries into precise owners. | None for file-level split classification. |
| Publication target-spelling group | `keep-local` | Register alias/view selection, frame-slot address spelling, scalar load/store mnemonics, fixed-frame-pointer selection, and relocation/register-indirect operands are AArch64 instruction spelling. | Stay local, likely under a non-dispatch publication/address helper after contraction. | None. |
| Publication prepared-consumer group | `keep-local` | Prepared local load offsets, block-entry/value-home checks, prepared branch/scalar facts, before-return ABI lookup, store-source planning, and retargeting now consume prepared/shared facts after idea 48. | Stay local as target publication emission over shared facts. | None. |
| Publication local-slot/address offset group | `needs-more-evidence` | `local_slot_address_frame_offset` remains a null implementation while `local_aggregate_address_frame_offset` and frame-address materialization consume prepared frame-address offsets. Prior closures repaired the known fallback families but this exact null helper may be dead, incomplete, or intentionally disabled. | Do not contract yet; create a narrow evidence/proof idea if Step 3 needs to dispose of it. | Need caller/runtime proof for local-slot-address cases that would hit the null path. |
| Publication value/register read hazard group | `keep-local` | `value_publication_may_read_register_index` and prepared-home register-read checks choose clobber-safe AArch64 publication sequencing, not semantic source ownership when they stay anchored in prepared facts. | Stay local. | None. |
| Publication BIR result-value helpers | `fold-back` | `instruction_result_value` and `instruction_result_value_ref` are generic BIR result extraction utilities, not AArch64 emission policy. | Fold into shared BIR query or inline at call sites. | None. |
| `dispatch_value_materialization.hpp` / `dispatch_value_materialization.cpp` file owner | `split-owner` | The public `emit_value_publication_to_register` is called across ALU, memory, calls, cast/comparison, FP materialization, publication, and edge paths; the file mixes prepared fact consumption with recursive local materialization. | Keep target value emission as a precise local owner; split semantic source discovery to shared/prepared authority. | None for file-level split classification. |
| Value-materialization prepared global/load-local/local-slot consumers | `keep-local` | `emit_prepared_global_load_to_register`, prepared local load offset/recovered-source consumption, value-home publication, and local-slot address publication emit AArch64 instructions from prepared memory/address facts after idea 49. | Stay local as emission over prepared facts, potentially under a `value_materialization` owner. | None. |
| Value-materialization recursive same-block/source discovery | `move-forward` | `prepared_same_block_scalar_producer` is a prepared consumer, but recursive operand materialization still relies on same-block producer records and direct `mir::evaluate_same_block_integer_constant` checks; idea 49 rejects deeper same-block producer recursion as authority. | Add/consume shared scalar materialization/source-producer facts; keep recursive emission mechanics local only after semantic source is shared. | Need caller coverage for non-edge/publication consumers. |
| Value-materialization scratch/write hazard group | `keep-local` | `value_publication_may_write_scratch_register` protects target scratch usage and recursive emission order; scratch/clobber checks are explicitly preserved by idea 49 out-of-scope rules. | Stay local. | None. |

## Suggested Next

Proceed to `plan.md` Step 3 by grouping the classifications above into bounded
follow-up ideas: mechanical fold-back wrappers, shared-authority migrations,
target-owner splits, and narrow evidence-only probes for the unresolved
local-slot/fallback paths.

## Watchouts

- This plan remains audit-only until a follow-up idea is activated.
- Follow-up implementation ideas should not merge every `dispatch*` file into
  `dispatch.cpp`; several rows above are `split-owner`, not bulk fold-back.
- Treat predecessor-depth scans, same-block producer recursion, direct-global
  select-chain dependency discovery, and join parallel-copy cache rebuilding as
  shared-authority candidates unless a narrow proof shows they are convenience
  wrappers only.
- Preserve target-local scratch/clobber handling, register spelling,
  instruction spelling, frame-address rendering, diagnostics, and final
  machine-instruction sequencing.
- `needs-more-evidence` currently applies to the publication local-slot null
  path and to fallback coverage under several `move-forward` rows; Step 3
  should keep those as evidence/proof ideas rather than contraction promises.

## Proof

Audit-only packet. No build or test required; no `test_after.log` update made.
Evidence gathered from Step 1 notes, source idea 59, closure notes for ideas
47, 48, 49, and 58, `c4c-clang-tool-ccdb function-signatures` for all six
dispatch-family `.cpp` files, and targeted caller/reference searches for the
same-block producer, edge fallback, select-chain, direct-global call-argument,
join parallel-copy, and value-publication helpers.
