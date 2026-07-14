# Typed-Authority Handoff To Idea 734

This is the plan-Step-6 resumable handoff from idea 741 to the open consumer
`ideas/open/734_lir_to_new_bir_container_completeness.md`, checked at HEAD
`e7a24c93d`. It records producer-side capability only. It does not claim any
new-BIR instruction or non-void terminator implementation.

## Capability now available

The shared `LirOperand` authority is exactly `monostate`, `LirValueId`,
`LinkNameId`, or `LirIntegerImmediate`. `fresh_value(ctx)` allocates from the
owning `LirFunction`; selected globals retain their HIR `LinkNameId`; integer
payloads are created from native HIR values; `LirGepIndex` carries a typed
operand; and `LirRet` owns an optional operand plus `LirTypeRef` despite its
compatibility field spellings.

For the authoritative shapes below, display is presentation-only. The LIR
verifier resolves value/global ownership and native immediates without parsing
`%tN`, `@name`, numeric spelling, typed index fragments, or printer output.

## Exact idea-734 rows unblocked

| Idea-734 coverage row | Typed LIR facts now available | Receiver work still owned by 734 |
|---|---|---|
| `LirInst::LirStoreOp`, direct selected-global scalar integer subrow | `type_str: LirTypeRef`; `ptr: LinkNameId` naming exactly one `LirGlobal`; `val: LirIntegerImmediate` representable by the type | Add a typed Raw-BIR `Store` payload and ordered use edges; map the global ID and materialize the immediate; add builder/view/verifier/import dispatch and transactional positive/negative tests |
| `LirInst::LirLoadOp`, direct selected-global scalar subrow | `type_str: LirTypeRef`; `ptr: LinkNameId` naming exactly one `LirGlobal`; `result: LirValueId` allocated by the current function | Add typed Raw-BIR `Load`; map the global ID; create/register one BIR instruction result keyed by the source ID; verify type/use/result ownership and transactionality |
| `LirInst::LirGepOp`, selected-global array-decay subrow | `element_type: LirTypeRef`; `result: LirValueId`; base `ptr: LinkNameId`; ordered all-typed `LirGepIndex` values with native integer or current-function SSA authority; native `inbounds` | Add typed Raw-BIR `GetElementPtr`; preserve ordered typed path and flag; map result/base/index authorities; verify shape/type/ownership and transactional failure. `index_adapter.hpp` proves authority-first mechanical adaptation but is not active importer receipt |
| `LirTerminator::LirRet`, scalar integer value subrow | `type_str: LirTypeRef`; `value_str` contains `LirIntegerImmediate` or a current-function `LirValueId`; void has no value | The checked-in `ReturnTerm` already has `optional<ValueId>`, but 734 must materialize or look up the value, enforce agreement with the imported function return type, wire the terminator, verify it, and test rollback. The active importer still rejects any value/non-void type as `InvalidVoidReturn` |

These rows unblock the receiver's typed-container design and wiring. They do
not make the whole modern operation alternative importable: each row is the
exact ordinary producer shape stated above.

## Existing receiver foundations that may be reused

- Direct scalar function signature receipt already lowers the relevant return
  type before body validation.
- Raw BIR already has generic function-local `ValueId`, `ValueDef`, instruction
  result/use edges, constants, module globals, and `ReturnTerm::value`.
- The current importer already maintains source-value and ordinary-value maps
  for its bounded constant/inline-asm slice. Idea 734 owns extending one
  coherent registry to the new instruction rows; 741 did not add a receiver
  side table.
- LIR verification runs before successful receipt and supplies invalid,
  duplicate, unknown, cross-function, unresolved-global, ambiguous-global,
  authority-alternative, type-parity, and immediate-range rejection.
  Importer verification must still validate its own BIR graph and publish
  transactionally.

## Boundaries that remain outside idea 741

The final authority matrix names every such variant individually. In
particular, local/SSA pointer forms, non-integer store values, non-global GEP
bases and ordinary GEP producers, non-integer returns, casts, arithmetic,
calls, PHIs, vector operations, varargs, allocas, memory intrinsics, inline-asm
binding identities, and raw control-flow labels may still be `monostate` or
raw compatibility. Idea 734 must not parse those displays. It may keep an exact
subrow unsupported while its receiving container is absent, and a future
producer-identity initiative is required before any still-raw semantic row can
be received losslessly.

Legacy ID-backed alternatives, direct-call `LinkNameId`, fieldless
`LirUnreachable`, and existing void-return/jump/inline-asm/constant receipt are
separate existing facts; 741 neither replaced nor claimed them.

## Proof and unchanged active boundaries

- `frontend_lir_call_type_ref` directly inspects native authorities for the
  four producer seams, neighboring cases, misleading displays, coercions, and
  malformed ownership/type states.
- `backend_lir_to_bir_interface` proves native GEP index adaptation wins over
  misleading display, authoritative SSA display aliases are not consulted,
  and authoritative scalar return remains outside current receipt.
- Focused LLVM output preserves the expected presentation: `store i32 7, ptr
  @lir_identity_scalar`, `%t0 = load i32, ptr @g_counter`, `%t0 =
  getelementptr [1 x i32], ptr @lir_identity_array, i64 0, i64 0`, and `ret i32
  0`. These strings are observations, not authority evidence.
- Focused semantic-BIR probes remain fail-closed: store/load/GEP report
  `UnsupportedOrdinaryInstruction`; scalar return reports
  `InvalidVoidReturn`.

## Resume point

## Direct-branch receiver return point (idea 747)

`LirBr.successor` now carries the sole semantic destination as a
current-function `LirBlockId`. `target_label` is retained only as the display
shadow of the block selected by that ID. Direct-target objects are allocated
when generated control-flow targets are scheduled, threaded through the direct
branch helpers, and used again when the destination block opens; scheduled HIR
blocks use their exact structural IDs. No producer, verifier, or receiver
looks up, parses, or reconstructs a successor from label text.

`verify_module` rejects a missing or invalid successor, an ID not owned by the
current function (including an ID found only in another function), duplicate
same-function ownership, and a display label that disagrees with the
successor-selected block. `frontend_lir_call_type_ref` exercises a native
valid `LirBr` plus those malformed neighbours. Focused proof:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$' > test_after.log`

Idea 734 resumes at its recorded Step 6.3 direct-branch receiver packet: map
this carried `LirBlockId` directly to the typed Raw-BIR jump destination and
verify same-function ownership/rollback. It must not recover an edge from
`target_label`; conditional, switch, indirect, and phi work remain separate.

After lifecycle closure of idea 741, idea 734 can reactivate at its blocked
function-body Step 4 boundary. The smallest receiver sequence is generic
source-value/global/immediate mapping, then the exact store/load/GEP subrows,
then scalar `LirRet` value wiring. Each slice still needs typed Raw-BIR
containers, importer dispatch, BIR verification, neighboring malformed tests,
and module-transactional proof. Completion of this handoff is not completion
of idea 734.
