Status: Active
Source Idea Path: ideas/open/248_phase_f2_x86_riscv_prepared_boundary_completion_criteria_audit.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Build Boundary Classification Matrix

# Current Packet

## Just Finished

Step 2 from `plan.md` converted the Step 1 inventory into a boundary
classification matrix for every blocker group named by idea 247 and the active
plan. No implementation, tests, baselines, source idea, or `plan.md` changes
were made.

### Step 2 Boundary Classification Matrix

| Blocker row | Classification | Evidence or missing evidence | Remains public prepared authority? |
| --- | --- | --- | --- |
| `PreparedFunctionLookups::call_plans` public lookup group | Blocked public prepared authority | Declared in `src/backend/prealloc/prepared_lookups.hpp:16`; built in `prepared_lookups.cpp:1538`; consumed by x86 `consume_plans`/debug/module paths, AArch64 traversal/call lowering, and prepared lookup/helper tests. Step 1 found no direct riscv `call_plans` source consumer, so this cannot be called x86/riscv-portable semantic authority. Idea 247 said no lookup group was deletion/demotion-ready. | Yes. It is still public lookup authority and compatibility evidence until x86/AArch64 consumers move and the riscv gap is either proven irrelevant or filled by a route/BIR fact. |
| `PreparedBirModule::call_plans` module field and `find_prepared_call_plans(const PreparedBirModule&)` | Blocked public prepared authority | Declared in `src/backend/prealloc/module.hpp:53` with module-level lookup helper at `module.hpp:152`; shares the same consumer buckets as the lookup group. Step 1 evidence does not show all public consumers migrated away from this field. | Yes. It remains a public prepared module entry point, not a demotion candidate yet. |
| Route 6 scalar call-use/source identity facts adjacent to call plans | BIR-owned semantic fact, partially proven | Ideas 243 and 244 accepted narrow x86 scalar `i32` Route 6 status/`ConsumedPlans` evidence and preserved prepared fallback/wrapper output. Step 1 records no broad x86 wrapper migration and no riscv parity for this call-plan family. | No for the route fact itself, but yes for the prepared agreement/fallback surfaces that still publish the observable answer. |
| Direct-call wrapper output and `ConsumedPlans` rows | Retained public compatibility adapter | Idea 244 explicitly preserved public `ConsumedPlans`, prepared fallback, and direct-call wrapper output; Step 1 cites `tests/backend/bir/backend_x86_handoff_boundary_direct_extern_call_test.cpp:432-565`, x86 `ConsumedPlans` in `src/backend/mir/x86/prepared/prepared.hpp:13`, and route-debug wrappers in `x86.hpp:258-272`. | Yes. These rows are compatibility authority and must stay stable while semantic authority moves. |
| `PreparedFunctionLookups::memory_accesses` lookup group | Blocked public prepared authority | Declared in `src/backend/prealloc/prepared_lookups.hpp:18`, built at `prepared_lookups.cpp:1541`, and exposed through public lookup functions at `prepared_lookups.cpp:1605` and `:1647`. AArch64 and prepared helper tests consume it; riscv evidence is indirect through Route 3/source-memory status rows; Step 1 found no direct x86 source consumer in the targeted search. | Yes. It remains public prepared lookup authority because x86 and riscv direct consumer coverage is incomplete. |
| Route 3 memory/source identity facts adjacent to memory accesses | BIR-owned semantic fact, partially proven | Ideas 243 and 245 accepted Route 3 memory/source identity candidates and riscv Route 3 agreement diagnostics, but Step 1 records prepared status/fallback retention and no complete x86/riscv consumer migration. | No for the route fact itself, but yes for the prepared fallback/status rows until both targets consume the same fact family. |
| Prepared source-memory statuses and fallback rows | Retained public compatibility adapter | Idea 245 preserved prepared status strings and fallback behavior; Step 1 cites riscv `source_memory_access_status`/fallback rows in `backend_riscv_prepared_edge_publication_test.cpp:1385`, `:1468-1508`, and `:1528-1566`, plus prepared helper tests. | Yes. They remain compatibility authority and fail-closed oracle surfaces. |
| `PreparedFunctionLookups::edge_publications` lookup group | Blocked public prepared authority | Declared in `prepared_lookups.hpp:21`, built at `prepared_lookups.cpp:1544`, with public lookup declarations such as `publication_plans.hpp:576`. Step 1 found riscv, x86, AArch64, and prepared-helper consumers. Ideas 245 and 247 preserved prepared fallback/status surfaces and rejected field-group deletion readiness. | Yes. It is still public cross-target prepared authority, even though Route 4/5 facts now have adapter evidence. |
| Route 4/5 edge-publication identity facts | BIR-owned semantic fact, partially proven | Idea 243 named Route 4/5 edge-publication identity as a BIR semantic candidate, and idea 245 added riscv Route 5 edge/source agreement diagnostics. Step 1 still shows prepared `edge_publications` as the public consumer surface across targets. | No for the route fact itself; yes for prepared agreement/fallback/status publication until consumers no longer need the lookup group. |
| riscv `EdgePublicationMoveIntentStatus` and prepared publication status rows | Retained public compatibility adapter | Idea 245 preserved `EdgePublicationMoveIntentStatus`, prepared status strings, and exact riscv instruction text. Step 1 cites `src/backend/mir/riscv/codegen/emit.hpp:26-37`, `emit.cpp:491-699`, `:741-765`, and riscv prepared edge-publication tests. | Yes. They remain public compatibility/oracle authority for riscv edge-publication behavior. |
| x86 edge-publication prepared dispatch/status surfaces | Retained public compatibility adapter | Step 1 cites x86 `EdgePublicationMoveIntentStatus` in `src/backend/mir/x86/prepared/prepared.hpp:154-186`, dispatch use in `src/backend/mir/x86/prepared/dispatch.cpp:70`, and x86 module output in `module.cpp:2480-2547`. | Yes. They remain compatibility authority until x86 consumes route/BIR edge facts without prepared as the observable answer. |
| `PreparedFunctionLookups::edge_publication_source_producers` lookup group | Blocked public prepared authority | Declared in `prepared_lookups.hpp:22`, built at `prepared_lookups.cpp:1545`, and used heavily by AArch64 lowering plus prepared helper tests. Step 1 found no direct riscv or x86 source-producer lookup use in targeted searches. | Yes. Public prepared authority remains because there is active AArch64/prepared compatibility use and missing x86/riscv parity evidence. |
| Edge-publication source-producer identity fact | BIR-owned semantic fact candidate, blocked | The fact shape is adjacent to Route 5 edge/source agreement, but Step 1 evidence only proves strong AArch64/prepared helper consumption and does not identify direct x86/riscv consumers of the source-producer lookup. | No as an independent public route fact today; yes for the prepared lookup that still carries the public answer. |
| `PreparedBirModule::module` | Blocked public prepared authority | Declared at `src/backend/prealloc/module.hpp:39`; AArch64 traversal reads `prepared.module.functions`, x86 accepts the whole prepared module, and tests mutate BIR through prepared modules. There is no Step 1 evidence that targets no longer need the embedded BIR module through the public prepared aggregate. | Yes. The field remains a public prepared aggregate surface, not deletion-ready. |
| Underlying BIR module/function/block structure | BIR-owned semantic fact | The field stores the BIR module produced by `prepare_semantic_bir_module_with_options` / `prepare_bir_module_with_options`; AArch64 traversal uses it for BIR function lookup. | No for the semantic structure itself, but yes for the public prepared aggregate wrapper around it. |
| `PreparedBirModule::names` | Blocked public prepared authority | Declared at `module.hpp:43`; Step 1 found x86 name resolution/route-debug consumers, AArch64 traversal and formatting/lookup use, prepared printer use, and lookup construction use. It mixes semantic name/index lookup with diagnostic and formatting compatibility. | Yes. It remains public prepared authority until semantic-name lookup and diagnostic/formatting uses are split. |
| Name-to-id/name-resolution semantic index | BIR-owned semantic fact candidate, blocked | `names` participates in BIR lookup and prepared lookup construction, but Step 1 did not show a route/BIR-owned replacement consumed by both x86 and riscv. | No as a standalone route fact today; yes via the public `names` field. |
| Name rendering, debug text, and formatting use of `names` | Retained public compatibility adapter | Step 1 records x86 route-debug, AArch64 formatting/lookup, and prepared printer consumers. Formatting/debug strings are compatibility surfaces, not deletion proof. | Yes. These uses remain prepared/public compatibility authority. |
| `PreparedBirModule::control_flow` | Blocked public prepared authority | Declared at `module.hpp:44`; x86 consumes it in `x86.hpp:17`, `:71-80`, `:169-170`; AArch64 consumes it in traversal/module paths; prepared lookup helpers build dominance/block facts from it. No riscv parity or public-consumer migration is recorded. | Yes. It remains public prepared authority because target and helper consumers still depend on it. |
| Control-flow graph/dominance/block-index facts | BIR-owned semantic fact candidate, blocked | Step 1 names dominance and block helpers (`prepared_block_index_by_label`, `prepared_block_successors`, `make_prepared_dominance_matrix`, `prepared_bir_function`, `prepared_bir_block`) but does not show route/BIR ownership consumed across x86 and riscv without prepared lookup helpers. | No as a completed route fact; yes through prepared control-flow helpers today. |
| `PreparedBirModule::store_source_publications` | Blocked public prepared authority | Declared at `module.hpp:54`; produced in `publication_plans.cpp:1705` and `:1823`; prepared printer consumes it, and source-memory compatibility is tested through prepared lookup helper/riscv status rows. Step 1 found no direct x86/riscv source consumer of the field, so evidence is indirect. | Yes. It remains public prepared authority because the only observed target-facing behavior is still through prepared source-memory/status compatibility. |
| Store-source publication semantic record | BIR-owned semantic fact candidate, blocked | The record is a source/publication semantic fact, but Step 1 did not identify direct x86/riscv consumers of the module field or a route-owned replacement. | No as a completed route fact; yes via `store_source_publications` and related prepared status rows. |
| Prepared lookup helper/oracle status names | Retained public compatibility adapter | Step 1 cites prepared lookup statuses in `prepared_lookups.cpp:464-498`, `:1380-1492`, `:1824-1962`, `:2436` and compatibility tests in `backend_prepared_lookup_helper_test.cpp:11663-11702`. Ideas 243 and 246 explicitly retained helper/oracle compatibility authority. | Yes. These are public oracle/status surfaces and must not be renamed or hidden as proof of ownership transfer. |
| Fallback behavior and unsupported fail-closed behavior | Retained public compatibility adapter | Ideas 243 through 246 preserve prepared fallback and unsupported fail-closed rows; Step 1 cites x86 and riscv fallback/status tests and target consumers. | Yes. Fallback remains prepared compatibility authority until replacement facts prove matching fail-closed behavior. |
| x86 route-debug output | Retained public compatibility adapter | Step 1 cites x86 route-debug entry points in `x86.hpp:258-272`, debug output in `debug.cpp:150-263`, and prior closure rows that retained x86 route-debug diagnostics. | Yes. Route-debug output is a compatibility/oracle surface, not deletion proof. |
| Wrapper assembly/output baselines | Target-owned policy product plus retained compatibility adapter | Idea 244 preserved direct-call wrapper output; Step 1 lists wrapper output beside target policy and compatibility surfaces. Wrapper behavior includes target ABI/layout/emission policy and stable public expected output. | Yes for compatibility output; no for target policy itself, which belongs to the target backend. |
| ABI, layout, register, stack, and storage surfaces | Target-owned policy product | Ideas 243 and 248 state ABI, layout, registers, stack, scratch registers, and storage remain outside BIR. Step 1 cites x86 ABI/frame/storage/addressing/rendering wrappers, AArch64 ABI/register/stack/storage lowering, and riscv register move/address materialization. | No. These should not become public prepared semantic authority or BIR ownership; prepared may only carry compatibility/fallback evidence. |
| Addressing, carrier/runtime-helper, formatting, emission, instruction spelling, and wrapper behavior | Target-owned policy product | Step 1 target-policy search recorded x86/riscv/AArch64 lowering and emission consumers; ideas 243 and 248 require these policies to stay outside BIR ownership. | No for policy decisions. Public prepared authority remains only for compatibility strings/baselines that observe these policies. |
| `PreparedBirModule::route` | Private prepared pass context candidate | Declared at `module.hpp:41`; Step 1 found prepared printer use at `prepared_printer.cpp:14-33` and no direct x86/riscv public consumer. | Not currently as target-facing public authority. Candidate for demotion once printer/diagnostic compatibility needs are specified. |
| `PreparedBirModule::invariants` | Private prepared pass context candidate | Declared at `module.hpp:42`; Step 1 found prepared printer use and no direct x86/riscv public consumer. | Not currently as target-facing public authority. Candidate for private pass-context demotion after compatibility output review. |
| `PreparedBirModule::completed_phases` | Private prepared pass context candidate | Declared at `module.hpp:64`; Step 1 found prepared printer use and no direct x86/riscv public consumer. | Not currently as target-facing public authority. Candidate for private pass-context demotion after printer/status compatibility is accounted for. |
| `PreparedBirModule::notes` | Private prepared pass context candidate | Declared at `module.hpp:65`; Step 1 found prepared printer use and no direct x86/riscv public consumer. | Not currently as target-facing public authority. Candidate for private pass-context demotion after printer/status compatibility is accounted for. |
| `PreparedBirModule::liveness` | Private prepared pass context candidate, blocked | Declared at `module.hpp:48`; Step 1 found substantial prealloc/regalloc/helper planning consumers in `call_plans.cpp`, `f128_runtime_helpers.cpp`, and `i128_runtime_helpers.cpp`. It is not simple metadata. | Not shown as direct target-facing public authority, but it is active prepared planning authority and cannot be demoted with the metadata fields. |

## Suggested Next

Execute Step 3 from `plan.md`: define x86/riscv portability readiness criteria
for each shared fact family, using the matrix above to separate route/BIR
semantic facts from retained prepared compatibility and target-owned policy.

## Watchouts

- No row above is deletion-ready. The only demotion candidates are the simple
  private metadata fields (`route`, `invariants`, `completed_phases`, `notes`),
  and even those need printer/status compatibility review before action.
- `liveness` is intentionally split from the simple metadata fields because it
  has active prealloc/regalloc/helper-planning consumers.
- `call_plans`, `memory_accesses`, `edge_publications`, and
  `edge_publication_source_producers` remain public prepared authority despite
  adjacent route/BIR fact candidates; current evidence is still partial or
  target-asymmetric.
- Target policy rows must stay outside BIR ownership. Prepared compatibility
  baselines can observe target policy, but they do not transfer that policy to
  BIR.

## Proof

No build/test proof required by the delegated packet. No new spot-check
commands were run for Step 2; the matrix uses the Step 1 inventory already
recorded in canonical `todo.md`.

No `test_after.log` was produced because the packet explicitly requested no
build/test proof and no root-level log files.
