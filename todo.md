Status: Active
Source Idea Path: ideas/open/239_phase_e5_prepared_bir_module_demotion_or_retirement_gate.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Map PreparedBirModule and PreparedFunctionLookups Field Groups

# Current Packet

## Just Finished

Step 2 mapped `PreparedBirModule` and `PreparedFunctionLookups` field groups
from the active Step 1 inventory, durable E0-E4 documents, lookup readiness
map, and current source surfaces. No implementation files, `plan.md`, source
idea, E5 document, expectations, or baseline files were changed.

Source surface anchors:

- `src/backend/prealloc/module.hpp` defines `PreparedBirModule` with the
  aggregate BIR payload, target profile/route metadata, names/control-flow,
  value locations, stack/frame/addressing/liveness/regalloc, call/publication,
  variadic/storage/carrier/atomic/intrinsic/inline-asm/runtime-helper,
  completed-phase, and note fields. The same header exposes public
  `find_prepared_*` helper families for register overrides, addressing, value
  locations, frame/dynamic-stack/call/variadic/storage plans, carriers,
  atomics, intrinsics, inline asm, and runtime helpers.
- `src/backend/prealloc/prealloc.cpp` constructs and mutates the aggregate via
  `BirPreAlloc::run()` and `publish_contract_plans()`, including stack,
  liveness, out-of-SSA, regalloc, labels, frame, dynamic stack, calls,
  store-source publications, variadic, storage, carriers, atomics,
  intrinsics, inline asm, and runtime helpers.
- `src/backend/prealloc/prepared_lookups.hpp` defines the seven
  `PreparedFunctionLookups` groups and the construction APIs
  `make_prepared_function_lookups(...)` and
  `make_prepared_move_bundle_lookups(...)`.
- `src/backend/prealloc/prepared_lookups.cpp` builds per-function lookup
  bundles from `PreparedBirModule`, control flow, call plans, value locations,
  addressing, value-home lookups, move bundles, source producers, and edge
  publications.
- Current consumers keep public prepared visibility through AArch64
  `FunctionLoweringContext` and `lower_prepared_functions(...)`, x86
  `ConsumedPlans` and route-debug compatibility paths, riscv prepared
  emission, `prepare::print(const PreparedBirModule&)`, prepared lookup helper
  tests, wrapper/debug tests, and expected-string/baseline authority.

`PreparedBirModule` field-group matrix:

| Field group | Current owner | Current consumers | Candidate disposition | Required adapters | Missing proof |
| --- | --- | --- | --- | --- | --- |
| `module`, `names`, selected `control_flow`, selected semantic route facts | BIR/route semantics plus prepared compatibility delivery | AArch64 lowering, x86 lookup by prepared names, printer/debug, route/helper oracles, lookup construction | Potential one-consumer semantic demotion only; retain aggregate wrapper | BIR identity/CFG/route-fact adapters with prepared agreement and fallback | Every selected consumer must prove direct BIR/route read, byte-stable diagnostics, and no hidden target-policy dependency. No whole-module demotion proof exists. |
| `target_profile`, `register_group_overrides`, `regalloc`, target-policy parts of `value_locations` | Target/prepared policy | AArch64/x86/riscv lowering, wrappers, register/home rendering, prepared printer/tests | Retain as target/prepared policy | Explicit target-policy owner only if future work extracts one narrow API | No cross-target proof that ABI/register/home policy can move into BIR or route facts. |
| `stack_layout`, `frame_plan`, `dynamic_stack_plan`, `storage_plans` | Target/prepared frame, stack, and storage policy | AArch64 contexts, x86 frame/storage consumers, printer/debug, stack/frame/home tests | Retain | Future target frame/storage owner with compatibility facade, if any | Missing byte-stable frame/stack/storage diagnostics and output proof; no route-owned semantic replacement. |
| `addressing` and address-materialization policy | Target/prepared address policy | AArch64 memory/address/call readers, x86/riscv wrapper-adjacent paths, address/materialization tests | Retain | One semantic source adapter only if it excludes address policy | Missing proof that any consumer can drop prepared address formation, relocation, legality, volatile/address-space, and final operand policy. |
| `liveness` | Mixed BIR annotation candidate plus transient pass context | Prealloc/regalloc, lowering context, diagnostics that combine liveness with stack/allocation facts | Blocked candidate for later field-specific audit | BIR use/def/live annotation adapter plus prepared diagnostic fallback | Missing consumer-by-consumer agreement proof and separation from allocation/stack policy. |
| `call_plans` | Target/prepared call policy with selected Route 6 semantic source subfacts | AArch64 call lowering/oracles, x86 `ConsumedPlans` and route-debug, wrappers, call tests | Only one-reader Route 6 source adapters are plausible; retain call plans | Route 6 call-use source adapter with prepared call-plan fallback | Idea 238 proves only x86 Route 6 scalar `i32` route-debug / `ConsumedPlans` compatibility. Missing broad call-wrapper, ABI/result-lane, riscv, and aggregate retirement proof. |
| `store_source_publications` and selected publication/source chains | Mixed route semantic candidates plus retained publication policy | AArch64 dispatch/memory/calls/publication, x86/riscv publication wrappers, printer `select_chains`, helper oracles | One route family or row at a time; retain publication mechanics | Route 1/2/4/5/6/7 source/publication adapter with prepared fallback | Missing cross-target wrapper proof, publication construction/move/home/storage policy proof, and byte-stable oracle coverage for non-agreement paths. |
| `variadic_entry_plans`, `i128_carriers`, `f128_carriers`, `atomic_operations`, `intrinsic_carriers`, `inline_asm_carriers`, `f128_runtime_helpers`, `i128_runtime_helpers` | Target/prepared/helper protocol policy, with possible narrow intrinsic metadata candidates | AArch64/x86/riscv lowering, helper/call protocol paths, printer/debug/tests | Retain; only future one-field semantic metadata audit could narrow intrinsic facts | Field-specific helper/protocol adapters preserving prepared fallback | Missing proof for carrier/helper completeness, feature policy, operand/result homes, runtime protocol, and expected output. |
| `route`, `invariants`, `completed_phases`, `notes` | Private pass context plus diagnostic/string/oracle authority | Prepared printer/debug, route-debug, helper-oracle rows, fallback/mismatch reporting | Retain as diagnostic/oracle authority | Row-scoped diagnostic adapters with prepared fallback | Missing byte-stable success and failure-path coverage for any broad diagnostic replacement; baseline/string authority must remain prepared-visible. |

`PreparedFunctionLookups` group matrix:

| Lookup group | Current owner | Current consumers | Candidate disposition | Required adapters | Missing proof |
| --- | --- | --- | --- | --- | --- |
| `call_plans` | Target/prepared call policy plus selected Route 6 source identity | AArch64 call lowering/oracles; x86 `ConsumedPlans`, scalar source helper, wrappers; prepared lookup tests | One Route 6 call-use source adapter at a time; no group deletion | Adapter must name one call instruction and argument/result role, compare route/prepared source, and fall back for policy-sensitive facts | Missing broad ABI/layout/result-lane/call-wrapper/riscv proof. Idea 238 remains narrow to x86 scalar `i32` route-debug / `ConsumedPlans`. |
| `address_materializations` | Target/prepared address policy | AArch64 address/materialization/frame-offset readers and prepared address tests | Retain; no current demotion candidate | None currently; any future adapter must isolate a semantic question from target address policy | Missing evidence that readers avoid frame/global/TLS relocation, offsets, legality, volatile/address-space, final operands, and sequencing. |
| `memory_accesses` | Mixed Route 3 memory/source semantic candidate plus retained address/materialization policy | AArch64 memory/call/dispatch/store-source readers, x86 local/compare candidates, memory/frame/stack tests | One AArch64 Route 3 memory/source reader adapter first; no group deletion | Route 3 adapter for memory access id/source identity with prepared mismatch fallback | Missing address-policy exclusion, failure-mode proof, and x86/riscv reuse proof after AArch64. |
| `move_bundles` | Target/prepared move scheduling and transient pass context | AArch64 edge-copy/call/return/dispatch/publication/prologue readers; x86 move/return wrappers; move tests | Retain; only future identity-only audit possible | Source-id adapter only if the consumer ignores move policy | Missing proof excluding move ordering, out-of-SSA placement, cycle temporaries, homes/storage, ABI phases, execution sites, final records, and output. |
| `value_homes` | Target/prepared home/storage/rendering policy | AArch64 operands/prologue/dispatch/publication/memory/calls; x86 decoded-home/formal-publication/register-home helpers; riscv operand rendering; value-home oracles | Retain; only future one-reader identity audit possible | Identity adapter with prepared home fallback | Missing proof that any consumer can ignore storage authority, register choice, stack slot/offset, rematerialization spelling, rendering, and value-home policy. |
| `edge_publications` | Mixed Route 4/5 semantic identity candidates plus retained publication/emission policy | AArch64 dispatch/memory/calls/publication, x86 wrappers, riscv prepared emission, publication tests/oracles | One semantic edge-publication consumer or wrapper input at a time; no group deletion | Adapter for predecessor/successor or block-label agreement, destination/source agreement, duplicate/conflict rejection, and prepared fallback | Missing unchanged move/register/stack/output proof, x86/riscv wrapper proof, and broad cross-target compatibility. |
| `edge_publication_source_producers` | Mixed Route 1/2/5/6/7 semantic candidates plus retained production fallback and printer/debug oracle | AArch64 dispatch/publication/memory/calls/store-source helpers, prepared printer `select_chains`, source-producer tests, x86/riscv wrapper-adjacent paths, route-debug/oracles | Best candidate is one route-native helper or printer/oracle row for one route family; no group deletion | Route-family adapter with route/prepared agreement and byte-stable diagnostic fallback | Missing materialization/call/publication policy exclusion, non-agreement fallback proof, wrapper proof, and unchanged expected strings. |

Disposition summary:

- Whole `PreparedBirModule` retirement remains blocked by mixed semantic,
  target-policy, pass-context, diagnostic/oracle, wrapper, printer/debug,
  fallback, public-consumer, and baseline/string authority.
- Whole `PreparedFunctionLookups` retirement remains blocked because all seven
  groups are retained or mixed; none is deletion-, privatization-, facade-, or
  aggregate-replacement-ready.
- Demotion-ready evidence is limited to already closed narrow helper/API or
  row-scoped facts. Idea 238 is prerequisite-complete only for the x86 Route 6
  scalar `i32` route-debug / `ConsumedPlans` compatibility boundary and does
  not prove broad x86/riscv/cross-target wrapper migration or aggregate
  prepared retirement.

## Suggested Next

Execute Step 3 by drafting
`docs/bir_prealloc_fusion/phase_e5_prepared_bir_module_demotion_or_retirement_gate.md`
from the Step 1 inventory and Step 2 matrix, conservatively keeping draft 155
blocked or rewritten-only unless the durable E5 conclusions can name a narrow
field-group follow-up with retained adapters and proof.

## Watchouts

- Keep idea 238 narrow: closed, prerequisite-complete, but only for x86 Route 6
  scalar `i32` route-debug / `ConsumedPlans` compatibility. It is not broad
  x86 call-wrapper, riscv, route-wide wrapper, or aggregate retirement proof.
- Step 3 should not claim readiness for any field group that lacks
  cross-target, diagnostic/oracle, fallback, public-consumer, baseline, or
  expected-string proof.
- Keep target policy out of target-neutral BIR: ABI, frame, registers, stack,
  addressing, move scheduling, call/helper protocols, carrier/runtime-helper
  protocol, branch/output spelling, final assembler, emission order, and
  wrapper output remain target/prepared-owned unless a later source proves a
  narrower owner.
- Preserve helper-oracle names/status labels, expected strings,
  supported-path status, prepared printer/debug output, route-debug output,
  wrapper output, fallback behavior, and baselines. Expectation rewrites,
  helper renames, unsupported downgrades, baseline refreshes, facade-only
  moves, and named-case-only shortcuts remain reject signals.

## Proof

Docs/lifecycle-only Step 2 mapping packet. No implementation files changed, so
no build or CTest proof was required and no `test_after.log` was generated for
this packet. Local proof was limited to source/lifecycle inspection with
`git status --short`, text/source reads, and AST-backed header signature
queries via `c4c-clang-tool`.
