Status: Active
Source Idea Path: ideas/open/391_rv64_variadic_prologue_save_area_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify The Save-Area Publication Route

# Current Packet

## Just Finished

Step 2: Classify The Save-Area Publication Route is complete.

Selected route: add a guarded RV64 prepared variadic-entry prologue
publication route that consumes explicit prepared facts describing incoming
post-named GPR argument registers and their backing save/overflow-area slots.
For a one-named-GPR callee like the Step 1 representative, the route should
publish incoming `a1` into the prepared RV64 variadic overflow/save area before
any `va_start` helper initializes a `va_list` pointer to that area.

Implementation owner: RV64 prepared object emission for a variadic function
entry, before ordinary instruction emission and before
`fragment_for_prepared_variadic_va_start` stores the overflow-area pointer into
the destination `va_list`. The existing `va_start` materialization may keep
owning pointer initialization, but it must not infer payload authority from the
`va_list` destination address alone.

Required prepared facts for the supported route:

- `PreparedVariadicEntryPlanFunction` exists for the function and is RV64 ABI
  shaped: helper-free `register_save_area.required == false`,
  `overflow_area.required == true`, `overflow_area.align_bytes == 8`,
  `overflow_area.base_slot_id` and `base_stack_offset_bytes` are present, and
  the `va_list_layout` has exactly one pointer-sized `OverflowArgArea` field at
  offset 0.
- The entry plan, or an adjacent prepared publication table keyed by the entry
  plan, explicitly lists each incoming variadic GPR payload to publish. Each
  record must name the source ABI GPR home (`a1` through `a7` as applicable),
  the logical argument position after the named GPR count, the destination
  overflow/save-area slot or base slot plus byte offset, and the payload width
  and alignment. The Step 1 representative needs exactly one record:
  source `a1`, first post-named GPR variadic payload, destination
  `overflow_area.base_slot_id` / offset 0, size 8.
- Source-to-destination matching must be one-to-one. A destination byte range
  can have exactly one incoming GPR source, and an incoming variadic GPR source
  can have exactly one destination range.
- Destination ranges must be inside the prepared RV64 variadic backing area
  reached by `overflow_area.base_slot_id` / `base_stack_offset_bytes`, fit the
  object emission immediate/range constraints, and not alias the destination
  `va_list` object slot except through the intentional zero-size overflow-base
  sentinel shape already produced by the prepared stack layout.
- Publication must happen in the callee prologue while incoming ABI registers
  are still live. Later call-argument moves, frame-slot-address materialization,
  or local `StoreLocalPublication` facts are not valid substitutes.

Fail-closed owner: RV64 prepared object emission should reject the prepared
function with an `unsupported_variadic_entry_lowering`-style diagnostic when a
variadic entry needs incoming GPR publication but the prepared saved-GPR
publication facts are absent, duplicated, ambiguous, non-RV64-shaped, outside
the backing area, not stack-slot based, not 8-byte GPR payloads, or conflict
with the existing helper-free RV64 `va_list` layout. The diagnostic should name
the missing or ambiguous saved-GPR publication authority, not the later
`va_start` pointer-materialization route.

Why this does not reopen earlier ideas:

- Idea 386 owns ordinary frame-slot-address GPR call-argument lowering; this is
  callee-entry publication of incoming variadic ABI registers, not a caller
  argument-address move.
- Idea 387 owns same-module memory-return/sret calls; no return-memory or sret
  contract is involved.
- Idea 388 owns `llvm.va_end.p0`; this route runs before and independently of
  `va_end`, which remains a no-op helper path.
- Idea 389 owns `va_start` destination-address materialization; that path
  correctly points the `va_list` at the prepared backing area, but does not
  populate the payloads in that area.
- Idea 390 owns prepared-call `FrameSlotAddress` `va_list` value
  publication/copy; it copies initialized `va_list` pointer payloads into later
  call argument objects and does not own the backing variadic payload area
  consumed through those pointers.

## Suggested Next

Step 3 should add focused backend coverage for the selected route: accepted
coverage for one or more incoming post-named RV64 GPR variadic payloads being
stored into the prepared overflow/save area before `va_start`, plus
fail-closed coverage for absent, duplicate, mismatched, or out-of-area
saved-GPR publication facts.

## Watchouts

- Do not implement the Step 2 route by hard-coding `va-arg-13.c`, `test`,
  `dummy`, literal `1234`, `a1`, or offset 72. Those are evidence for the first
  representative, not route predicates.
- Current prepared RV64 variadic facts expose the overflow-base and `va_start`
  homes, but they still do not expose an explicit incoming-GPR publication
  table. Step 3/4 may need a new prepared fact producer before object emission
  can safely consume the route.
- The existing helper-free RV64 admission contract treats
  `register_save_area.required == false` as valid. Do not flip RV64 to the
  AAPCS64 register-save-area model; this route is RV64 incoming-GPR
  publication into the prepared overflow/save backing area.
- Address materialization and idea 390 local pointer-copy publications remain
  insufficient proof that the backing payload area was initialized.

## Proof

Todo-only route classification. No implementation files or tests were changed.
Delegated proof command:

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`
- Result: PASS. Backend subset completed successfully; `test_after.log` is the
  proof log.

Step 1 evidence paths remain:

- `build/agent_state/391_step1_va-arg-13.semantic-bir.log`
- `build/agent_state/391_step1_va-arg-13.prepared-bir.log`
- `build/agent_state/391_step1_va-arg-13.c4c-disasm.log`
- `build/agent_state/391_step1_va-arg-13.clang-disasm.log`
- `build/agent_state/391_step1_va-arg-13.case.log`
- `build/agent_state/391_step1_va-arg-13.qemu-strace.log`
- `test_after.log` is the refreshed root proof log for the delegated backend
  subset.
