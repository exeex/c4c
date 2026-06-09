Status: Active
Source Idea Path: ideas/open/138_call_plan_lookup_ownership_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Rewire definitions and aggregate construction

# Current Packet

## Just Finished

Step 3 inspected the definition and construction boundary after the Step 2
declaration move. No implementation rewrite was applied.

- The smallest behavior-preserving boundary is to keep
  `make_prepared_call_plan_lookups`, the call lookup key helpers, and the call
  lookup query definitions in `src/backend/prealloc/prepared_lookups.cpp`.
- Moving those definitions into a call-owned implementation file would pull in
  prepared module/BIR access, control-flow dominance helpers, and block-entry
  republication helpers that already live with the broader prepared lookup
  construction pipeline.
- `PreparedFunctionLookups::call_plans` aggregate construction is preserved as
  the single `make_prepared_call_plan_lookups(prepared, call_plans, function)`
  field initializer inside `make_prepared_function_lookups`.
- A local scan found no downstream consumer rebuilding the full call lookup
  maps manually. The only separate `PreparedCallPlanLookups` construction is
  the existing incremental prior-preservation lookup inside call-plan
  construction itself, before the finalized function lookup aggregate exists.

## Suggested Next

Execute Step 4 from `plan.md`: update or verify known consumers so each use of
call-plan lookup facts includes or reaches the call-owned declaration boundary,
then run the supervisor-selected build verification.

## Watchouts

- `calls.hpp` forward-declares `PreparedBirModule` and
  `PreparedControlFlowFunction`; Step 3 found no reason to add `module.hpp` or
  `control_flow.hpp` includes there.
- The builder/query definitions intentionally still live in
  `prepared_lookups.cpp`; moving them would widen ownership to helpers that are
  not call-only.
- Keep `PreparedFunctionLookups`, non-call lookup structs, and broad prepared
  lookup helper declarations in `prepared_lookups.hpp`.
- Do not change ABI classification, call lowering semantics, preservation
  behavior, or AArch64 register handling.
- Do not replace prepared call lookups with local scans in AArch64 consumers.

## Proof

Inspection-only Step 3 result: no implementation files changed, so no build
was required by the delegated proof contract. No root-level proof log was
created because the packet explicitly listed root-level logs under Do Not
Touch.
