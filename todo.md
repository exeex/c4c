Status: Active
Source Idea Path: ideas/open/20_aarch64_codegen_layout_classification.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Classify Calls, Memory, Comparison, Prologue, And Return Helpers

# Current Packet

## Just Finished

Completed `Step 3: Classify Calls, Memory, Comparison, Prologue, And Return
Helpers` by auditing the named helper families against the reference ARM owner
layout and idea 17's absent-selection fallback closure.

Closure evidence used:

- Idea 17 closed after local aggregate address and indirect byval lane payload
  fallback paths were retired or guarded. The replacement source of truth is
  prepared `LocalFrameAddressMaterialization`, direct `FrameSlotAddress`, and
  prepared `ByvalRegisterLane` selected-source facts, with incomplete facts
  failing closed through diagnostics.
- Idea 17 explicitly kept calls-file consolidation and new ABI classification
  out of scope. That prevents treating current calls helper boundaries as
  semantic migration debt merely because they are numerous.
- The Step 2 audit already classified `calls_dispatch_bridge.*` as calls-owner
  fold-back debt. Step 3 confirms that call-boundary materialization and
  publication are AArch64 calls/ABI responsibilities as long as source choice
  comes from prepared call plans.

| Family | Bucket | Concrete owner-file recommendation | Rationale and follow-up direction |
| --- | --- | --- | --- |
| `calls_byval_aggregates.cpp` | fold-back | Fold into `calls.cpp` or a calls-internal byval section when the calls owner is mechanically consolidated. | The helpers recognize prepared byval lane moves and translate complete `ByvalRegisterLane` source selections into AArch64 memory operands. Idea 17 says byval lane source choice now belongs to prepared facts, so this file is target-local operand adaptation rather than separate authority. |
| `calls_common.cpp` | fold-back | Fold shared calls helpers back into `calls.cpp`; if kept split during transition, treat it as calls-private support, not a standalone owner. | It owns stack argument sizing, fixed-frame/`va_start` offset helpers, prepared register conversion, F128 carrier checks, scalar views, and diagnostics. These are calls/ABI lowering utilities around prepared facts and have no reference-style owner outside `calls`. |
| `calls_dispatch_bridge.cpp`, `calls_dispatch_bridge.hpp` | fold-back | Fold into `calls.cpp` with the call lowering entry points, leaving dispatch as caller only. | The bridge consumes `PreparedCallPlan`, lowers before/after-call moves, materializes local aggregate addresses from prepared `LocalFrameAddressMaterialization`, handles dynamic-stack helper calls, and records call results. Its semantics are calls/ABI lowering; idea 17 confirms the former local-address fallback is now a prepared-source-selection diagnostic path, not a separate dispatch authority. |
| `calls_moves.cpp` | fold-back | Fold into `calls.cpp` or a calls-private move section during mechanical calls consolidation. | The file is large, but the observed ownership is call-boundary move emission: prepared move bundles, ABI register/stack placements, preservation republication, before/after-call moves, before-return moves, and target register spelling. It consumes prepared move authority and should not be moved forward unless a later audit finds a concrete missing prepared field. |
| `memory_dynamic_stack.cpp`, `memory_dynamic_stack.hpp` | keep-local | Keep as a separate AArch64 dynamic-stack helper under the memory/call boundary, or fold into `memory.cpp` only as mechanical cleanup if the file stays small. | The family recognizes stack-save/alloca/restore helper calls, requires prepared dynamic-stack operation authority, and emits AArch64 `sp` manipulation with x16/x17 scratch policy and stable-frame-pointer diagnostics. The semantic operation plan is already prepared; the remaining behavior is target-local frame/stack emission. |
| `memory_store_sources.cpp`, `memory_store_sources.hpp` | needs-more-evidence | Run a focused memory-store source audit before deciding between fold-back into `memory.cpp` and a split where semantic producer/source-selection facts move to shared prepare. | The family clearly contains AArch64 store/load publication emission, pointer-base materialization, global-symbol materialization, and target scratch/register spelling. It also contains same-block producer predicates for narrow-store/wide-load, casts, selects, scalar FP binary producers, byval frame-slot loads, and pending store-global stack publications. Those query names are not enough to prove shared-authority debt, but they are enough to require a targeted audit before calling this purely mechanical. |
| `comparison_branch_fusion.cpp`, `comparison_branch_fusion.hpp` | fold-back | Fold into `comparison.cpp` as the AArch64 comparison/branch lowering owner, keeping dispatch integration through hooks only. | The helper consumes prepared branch-condition facts and emits AArch64 compare/branch spellings, scratch-register materialization, and same-block emission shortcuts. The semantic branch condition is prepared; the file is target-local compare/branch emission and does not justify a standalone reference-style owner. |
| `prologue_entry_formals.cpp` | fold-back | Fold into `prologue.cpp` under entry formal publication/prologue setup. | The file lowers prepared formal publication plans into AArch64 stores, loads, register moves, byval aggregate entry copies, and F128 entry handling. It computes AArch64 register slots and stack offsets for entry publication, but the owner is still prologue/ABI setup. No idea 17 evidence makes this shared prepare debt. |
| `returns.cpp`, `returns.hpp` | keep-local | Keep as the AArch64 `returns` owner matching the reference `returns` family; do not create a fold-back follow-up unless later cleanup is just internal slimming. | `returns.*` already matches a reference-style owner and owns return record construction, print-form classification, immediate/symbol materialization classification, return-use effects, and prepared return terminator lowering. It consumes prepared value homes and emitted scalar state; missing operand authority reports diagnostics rather than rederiving sources. |

Step 3 semantic authority findings:

- No Step 3 family is currently classified as `move-forward`.
- Idea 17's relevant source-selection authority is already in prepared call
  plans for local aggregate addresses and byval register lanes; AArch64 calls
  code should remain a consumer that diagnoses missing prepared facts.
- `memory_store_sources.*` is the only Step 3 family requiring more evidence
  before mechanical consolidation. The exact missing evidence is whether its
  same-block producer/source-publication predicates encode target-neutral
  memory source-selection facts that belong in shared prepare, or whether they
  only choose AArch64 emission sequences from facts already prepared elsewhere.

## Suggested Next

Start `Step 4: Build The Durable Classification Table` by merging the Step 1
inventory, Step 2 classifications, and Step 3 classifications into one narrow
table. Carry `compatibility_projection.*` and `memory_store_sources.*` as
`needs-more-evidence` unless the supervisor delegates the missing audits first.

## Watchouts

Do not turn idea 17's retired fallback debt into a new calls move-forward idea.
The closure note says the affected source-selection authority is now prepared
or fails closed, and calls-file consolidation was explicitly out of scope.

For Step 4, keep semantic migration and mechanical fold-back follow-ups
separate. `calls_*`, `comparison_branch_fusion.*`, and
`prologue_entry_formals.cpp` look like mechanical fold-back debt; only
`memory_store_sources.*` needs a semantic audit before its final follow-up
shape is safe.

## Proof

Not run. This was a classification-only packet, and the delegated proof
explicitly said not to run build/tests and not to modify `test_after.log`
because this step classifies existing files and the plan forbids root proof-log
changes.
