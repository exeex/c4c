Status: Active
Source Idea Path: ideas/open/20_aarch64_codegen_layout_classification.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Classify Dispatch And Publication Families

# Current Packet

## Just Finished

Completed `Step 2: Classify Dispatch And Publication Families` by auditing the
extra dispatch, publication, bridge, compatibility-projection, and
materialization families against closure evidence from ideas 16, 18, and 19.

Closure evidence used:

- Idea 16 closed with shared prepared edge-publication facts owning covered
  edge/block-entry value-flow authority. AArch64 keeps target-local emission,
  diagnostics, scratch, and hazard handling. Its close note explicitly says the
  remaining AArch64 producer lookup is not the authority path for prepared edge
  publications.
- Idea 18 closed as a target-local register-hazard repair for nested
  value/edge publication. The lesson is clobber avoidance during AArch64
  materialization, not a reopened shared-authority migration.
- Idea 19 closed after x86/RISC-V handoff review found no required shared
  prepared contract repair. It records that scratch selection, clobber
  avoidance, physical register spelling, register-class constraints, stack
  operand syntax, move instruction spelling, branch/control-flow emission, and
  final assembly formatting stay target-local.

| Family | Bucket | Proposed owner / follow-up direction | Owner rationale |
| --- | --- | --- | --- |
| `dispatch.cpp`, `dispatch.hpp` | keep-local | Keep as the AArch64 block-dispatch orchestrator; later mechanical slimming can move helpers out of the main dispatcher only when another owner is already classified. | This file coordinates prepared-block traversal, target instruction lowering, diagnostics, call bridging, publication hooks, and terminator emission. Closure notes do not identify the dispatcher itself as semantic authority debt after prepared edge-publication migration. |
| `dispatch_diagnostics.cpp`, `dispatch_diagnostics.hpp` | fold-back | Fold mechanically into `dispatch` or a narrow diagnostics section when the dispatch family is consolidated. | The helpers build AArch64 lowering diagnostics and unsupported-operation messages. They are target-local reporting utilities, not shared semantic facts, and have no separate reference ARM owner. |
| `dispatch_edge_copies.cpp`, `dispatch_edge_copies.hpp` | keep-local | Keep as the AArch64 edge-copy emission and hazard helper for now; future cleanup should split only if fresh evidence finds semantic producer rediscovery outside prepared facts. | Idea 16 moved edge/block-entry value-flow authority to shared prepared facts and says remaining AArch64 producer lookup is not the authority path. Idea 18 reinforces that clobber avoidance during edge/value publication remains target-local. |
| `dispatch_lookup.cpp`, `dispatch_lookup.hpp` | keep-local | Keep as the target-local lookup adapter around prepared value homes and local scalar state; do not create a move-forward idea without evidence that it owns new semantic facts. | The surface reads prepared ids/homes and same-block availability for emission decisions. Ideas 16 and 19 already validate shared prepared lookup authority, so this local adapter is consumer-side plumbing. |
| `dispatch_producers.cpp`, `dispatch_producers.hpp` | keep-local | Keep as AArch64 same-block producer analysis used by local materialization and hazard checks; only a later focused audit should move a helper forward if it proves to encode target-neutral semantics not already in shared prepare. | Closure evidence rejects testcase-shaped producer lookup as authority, but also states remaining AArch64 producer lookup is not the prepared edge-publication authority path. The observed helpers support emission choices such as constants, select chains, globals, and current-block join copy detection. |
| `dispatch_publication.cpp`, `dispatch_publication.hpp` | keep-local | Keep as AArch64 publication emission, retargeting, and clobber/hazard handling around shared prepared records. | Ideas 16 and 19 put publication facts in shared prepare while keeping scratch policy, register spelling, stack operands, and clobber avoidance in each target. Idea 18 specifically validates local hazard handling as necessary. |
| `dispatch_publication_common.hpp` | fold-back | Fold into `dispatch_publication` during mechanical cleanup. | The declarations are AArch64 publication helper utilities for register aliases, frame-slot addressing, scalar widths, and register views. They support publication emission but do not justify a standalone owner. |
| `dispatch_value_materialization.cpp`, `dispatch_value_materialization.hpp` | keep-local | Keep as target-local value materialization feeding publication registers; future cleanup may merge it into publication if file-boundary simplification is desired. | It emits AArch64 register materialization sequences from values using target registers and scratch registers. Idea 18 classifies this clobber-sensitive behavior as local emission responsibility. |
| `prepared_value_home_materialization.cpp`, `prepared_value_home_materialization.hpp` | keep-local | Keep as the AArch64 consumer of prepared value-home facts. | Shared prepare owns the home facts; this family owns AArch64 loads/moves from homes into physical registers and stack addresses. Idea 19 explicitly keeps physical registers, stack syntax, and move spelling target-local. |
| `constant_materialization.cpp`, `constant_materialization.hpp` | keep-local | Keep as AArch64 instruction-spelling support for integer constants. | Constant facts may be target-neutral, but MOV/MOVK-style integer materialization and register-width spelling are AArch64 emission details. No closure note supports moving this authority forward. |
| `fp_value_materialization.cpp`, `fp_value_materialization.hpp` | keep-local | Keep as AArch64 FP value/immediate materialization and publication to prepared FP registers. | The family chooses target FP/GP scratch behavior and AArch64 instruction lines, which idea 19 says remain target-local. |
| `compatibility_projection.cpp`, `compatibility_projection.hpp` | needs-more-evidence | Run a focused caller/projection audit before proposing removal, retention, or fold-back into module compilation/printing. Missing evidence: exact current callers, whether legacy MIR tests still require flat `CompatibilityProjection`, and whether all terminal assembly printing already walks `module::MachineModule` through the shared MIR printer. | The header labels the family compatibility-only for legacy MIR tests and migration callers, with terminal assembly expected to use shared MIR printing. That comment is directionally useful but not enough closure evidence to decide keep-local vs retire/fold-back. |
| `calls_dispatch_bridge.cpp`, `calls_dispatch_bridge.hpp` | fold-back | Classify as a Step 3 calls-owner fold-back candidate; keep any call-boundary publication and materialization semantics target-local unless a later calls audit finds shared prepare debt. | Although dispatch invokes this bridge, the file owns call-plan consumption, call-boundary materialization, preserved stack publication, and call-result recording. These are AArch64 calls/ABI responsibilities, and idea 19 keeps call-boundary scratch/clobber/emission details target-local. |

No Step 2 family is currently classified as `move-forward`. The relevant
semantic edge-publication authority from idea 16 has already moved to shared
prepared facts, and idea 19 found no required shared prepared contract repair.
The only unresolved Step 2 item is `compatibility_projection.*`, where the
missing evidence is caller/projection usage rather than closure-note evidence.

## Suggested Next

Start `Step 3: Classify Calls, Memory, Comparison, Prologue, And Return
Helpers`, with particular attention to the `calls_dispatch_bridge.*` fold-back
candidate recorded above so the calls owner classification remains coherent.

## Watchouts

Do not create a new move-forward idea for dispatch producer/lookup helpers
based only on file names. Ideas 16 and 19 say the shared prepared
edge-publication authority is already the boundary, and the remaining AArch64
lookup/producer helpers need fresh proof before being treated as semantic
authority debt.

Carry `compatibility_projection.*` as `needs-more-evidence` until a caller audit
answers whether the compatibility projection is still required by legacy tests
or migration callers.

## Proof

Not run. This was a classification-only packet, and the delegated proof
explicitly said not to run build/tests and not to modify `test_after.log`
because this step classifies existing files and the plan forbids root proof-log
changes.
