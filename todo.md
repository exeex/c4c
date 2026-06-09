Status: Active
Source Idea Path: ideas/open/141_prepared_lookups_residual_owner_audit.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Classify residual ownership

# Current Packet

## Just Finished

Step 4 residual ownership classification completed for every Step 1 public
group using the Step 2 definition map and Step 3 consumer map. Each group below
has exactly one primary category:

| Group | Primary category | Evidence and rationale |
| --- | --- | --- |
| Stack-layout id lookup helpers | `move-to-domain-owner` | Declarations are only `find_frame_slot_by_id` and `find_stack_object_by_id`. Definitions are simple `PreparedStackLayout` scans, not prepared aggregate construction. Consumers are AArch64 frame/address wrappers, memory retargeting, calls, and comparison helpers. Move declarations/definitions to `stack_layout/stack_layout.hpp` ownership. |
| Move-bundle index facts and maps | `move-to-domain-owner` | Declarations expose `PreparedMoveBundleLookups` and ABI move maps. Definitions build indexes from `PreparedValueLocationFunction::move_bundles` plus after-call result-lane publication from value homes. Consumers are AArch64 `alu.cpp`, `calls.cpp`, `memory.cpp`, `module.*`, and traversal field projection. Primary owner should be value-location/move-bundle ownership, with call-specific accessors allowed to include that owner rather than the facade. |
| Return-chain index maps | `keep-core-aggregate` | Declarations expose `PreparedReturnChainLookups`. Definitions scan return blocks and scalar binary chains; Step 2 found no concrete narrow owner beyond the broad control-flow neighborhood. Consumers are concentrated in AArch64 `alu.cpp`. No-new-idea note: keep this in the central prepared lookup aggregate until a real return-chain owner exists; do not create a vague move-only follow-up. |
| Value-home index maps | `move-to-domain-owner` | Declarations expose `PreparedValueHomeLookups`. Definitions are pure reverse-index construction over `PreparedValueLocationFunction::value_homes`. Consumers include AArch64, prealloc `formal_publications`, `decoded_home_storage`, and module context type exposure. Move to `value_locations.hpp` ownership so consumers can include the value-home owner directly. |
| Current-block join source status naming | `split-fact-from-routing` | Declarations name join-copy source status. Definitions assign status inside source-fact preparation and propagate it into instruction routing. Consumers use both source facts and routing in `dispatch_producers.cpp`. Status belongs with reusable join-copy source facts, not with routing-only AArch64 convenience. |
| Current-block entry publication status naming | `move-to-domain-owner` | Declarations name entry publication status. Definitions are query-result status over value-home and block-entry publication collection. Consumers are AArch64 publication dispatch. Move to value-location/publication ownership, with `value_locations.hpp` the strongest existing status owner. |
| Same-block producer query facts | `move-to-domain-owner` | Declarations include same-block scalar producer, materialization query, and current-block publication consumption structs. Definitions validate source-producer facts and same-block instruction order. Consumers are AArch64 dispatch/materialization, FP materialization, calls, and ALU. Move to source-producer/materialization ownership, likely `select_chain_lookups.hpp` plus publication facts rather than the central facade. |
| Call-argument materialization facts | `move-to-domain-owner` | Declarations expose call-argument source-producer materialization and opcode predicate. Definitions delegate through same-block scalar producer lookup with call-specific policy. Consumers are AArch64 calls. Move the public API toward `calls.hpp` while depending on the same-block/source-producer owner. |
| Fused-compare and condition producer facts | `move-to-domain-owner` | Declarations expose fused-compare operand and materialized-condition producer facts. Definitions are comparison/materialized-condition semantic logic over same-block producers and integer constants. Consumers are AArch64 `comparison.cpp`. Move to a comparison/control-flow comparison owner; ambiguity note: Step 2 did not find a dedicated comparison prealloc header, so the next plan should name or create that narrow owner explicitly. |
| Same-block load-local stored-value facts | `move-to-domain-owner` | Declaration exposes same-block load-local stored-value source lookup. Definition crosses same-block source producer, prepared memory access/addressing, and stack-layout ranges. Consumer pressure is AArch64 calls. Move toward memory-access/addressing ownership with source-producer dependency; do not keep it solely because it currently shares private helpers in `prepared_lookups.cpp`. |
| Current-block join parallel-copy source facts | `split-fact-from-routing` | Declarations expose reusable join-copy source fact structs/query inputs. Definition mixes publication/source/home facts with source/destination routing flags and instruction-value sets. Consumer `dispatch_producers.cpp` needs source facts and routing, but facts are reusable prepared information. Split publication/control-flow facts from target-routing convenience. |
| Current-block entry publication query result | `move-to-domain-owner` | Declarations expose query inputs/result and overloads. Definitions resolve value ids/homes and call block-entry publication collection. Consumer is AArch64 publication dispatch. Move to value-location/publication ownership with status naming. |
| Current-block join instruction routing convenience | `split-fact-from-routing` | Declaration exposes routing struct and helper. Definition calls source-fact preparation then marks per-instruction result booleans for the current block walk. Consumer is AArch64 `dispatch_producers.cpp`. Keep reusable facts elsewhere and move or localize routing convenience separately. |
| Function-level lookup aggregate | `keep-core-aggregate` | Declaration exposes `PreparedFunctionLookups` and `make_prepared_function_lookups`. Definition wires call plans, address materializations, memory accesses, move bundles, return chains, value homes, edge publications, and source producers in one pass. Consumers include AArch64 traversal/module, RISC-V, and x86. No-new-idea note: keep a central aggregate/builder; follow-ups should shrink declarations around it, not eliminate the aggregate. |
| Key helper functions | `move-to-domain-owner` | Declarations expose map key builders for move bundles and return chains. Definitions are hash-key construction. Consumers are internal construction/accessor code, not target policy. Move move-bundle keys with move-bundle lookup ownership; keep return-chain keys with the retained return-chain aggregate implementation until a concrete return-chain owner exists. |
| Lookup preparation helpers | `move-to-domain-owner` | Declarations expose builders for move bundles, return chains, value homes, edge publications, and source producers. Definitions show value-home/move-bundle builders are domain-index construction, edge-publication builders are publication construction, and source-producer builder is already defined in `select_chain_lookups.cpp`. Move value-home, move-bundle, edge-publication, and source-producer builders to their domain owners; keep return-chain builder with the core return-chain aggregate for now. |
| Value-home and out-of-SSA helper predicates | `split-fact-from-routing` | Declarations expose register-sharing and out-of-SSA destination/source predicates. Definitions support join-copy source facts and routing. Consumers are currently through join-copy preparation. Split value-home/register-sharing fact predicates from target-routing use; likely value-location/control-flow fact ownership rather than facade ownership. |
| Indexed move-bundle query helpers | `move-to-domain-owner` | Declarations expose indexed bundle, before-call, after-call, and before-return ABI lookups. Definitions are accessors over move-bundle maps with fallbacks to value-location move bundles. Consumers are AArch64 ALU, calls, memory, and module. Move with `PreparedMoveBundleLookups`; call/return consumers should include the move-bundle owner and any call owner needed for ABI semantics. |
| Return-chain query helpers | `keep-core-aggregate` | Declarations expose terminal/next operand lookups. Definitions read maps built by return-chain lookup construction. Consumers are AArch64 `alu.cpp`. No-new-idea note: leave these with `PreparedReturnChainLookups` in the central prepared lookup surface until a concrete return-chain/control-flow owner is designed. Ambiguity: evidence is insufficient to name a non-facade file today. |
| Publication source-producer lookup helper | `move-to-domain-owner` | Declaration remains in `prepared_lookups.hpp`, but definition is already in `select_chain_lookups.cpp`. Consumers include edge-copy dispatch, publication plans, same-block queries, and prepared printer select chains. Move declaration to `select_chain_lookups.hpp` or publication/source-producer owner. |
| Current-block publication consumption query | `move-to-domain-owner` | Declaration exposes same-block publication consumption query. Definition validates source-producer lookup, same-block order, instruction identity, and result-name match. Consumers are same-block materialization and call paths. Move with same-block/source-producer materialization ownership. |
| Same-block scalar and integer-constant query helpers | `move-to-domain-owner` | Declarations expose scalar producer and integer-constant evaluation overloads. Definitions are semantic same-block materialization logic over source producers. Consumers are AArch64 value materialization, FP materialization, calls, and ALU. Move to the same-block/source-producer materialization owner. |
| Stack-source publication helpers absent from this header | `no-new-idea` | Step 1 found no standalone residual public declaration in `prepared_lookups.hpp`; Step 2 located aggregate and typed same-width stack-source helpers in `publication_plans.hpp/.cpp`; Step 3 found AArch64 `memory.cpp` consumes the typed publication result while final emission remains AArch64-local. No-new-idea note: publication-plan ownership is already correct; do not reopen a prepared-lookup residual move for these helpers. |

Long-term `PreparedFunctionLookups` recommendation:

- Keep `PreparedFunctionLookups` and `make_prepared_function_lookups` as the
  central one-pass aggregate and field-projection boundary.
- Move domain lookup structs, key builders, accessors, and semantic query
  declarations out of `prepared_lookups.hpp` when an existing concrete owner is
  visible: `value_locations.hpp`, `publication_plans.hpp`,
  `select_chain_lookups.hpp`, `calls.hpp`, `addressing.hpp`, or
  `stack_layout/stack_layout.hpp`.
- Do not replace the aggregate with independent target rescans. AArch64,
  RISC-V, and x86 all show aggregate-level consumer pressure, and traversal
  wants one prepared-function construction pass.

Ambiguity notes:

- Return-chain ownership is insufficiently proven for a move. Evidence shows
  semantic return-chain logic, but no narrow owner file comparable to
  `calls.hpp`, `value_locations.hpp`, or `publication_plans.hpp`. Classify
  return-chain maps and query helpers as `keep-core-aggregate`.
- Fused-compare/materialized-condition facts are clearly not core aggregate
  construction, but Step 2 did not identify a dedicated prealloc comparison
  owner. A follow-up should name or create a comparison/control-flow comparison
  owner before moving declarations.
- Current-block join-copy groups need a split, not a bulk move. Reusable facts
  belong with publication/control-flow facts; per-instruction result routing is
  target-facing convenience.

## Suggested Next

Supervisor should hand this classification to plan-owner for the next lifecycle
decision: either generate a focused implementation plan from the concrete
`move-to-domain-owner`/`split-fact-from-routing` groups, or close/retire the
analysis runbook while preserving the return-chain and stack-source
`no-new-idea` decisions.

## Watchouts

- Do not treat broad AArch64 includes as primary owner evidence; classify from
  declarations, definitions, and specific consumers together.
- Preserve the distinction between aggregate retention and declaration moves:
  keeping `PreparedFunctionLookups` does not justify keeping every query helper
  in the facade.
- Avoid a vague return-chain move. Either design a concrete return-chain owner
  in a later plan, or leave the return-chain surface with the core aggregate.
- Do not reopen stack-source publication helper ownership; the residual audit
  found no `prepared_lookups.hpp` public group there.

## Proof

Inspection-only analysis slice. Used the Step 1 and Step 2 `todo.md` history,
the current Step 3 consumer map in `todo.md`, and targeted file/history reads
only. No build or test command was run, and no `test_after.log` was required by
the delegated packet.
