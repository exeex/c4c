# Idea 24 RISC-V Register Edge Publication Review

Active source idea: `ideas/open/24_riscv_prepared_edge_publication_register_destination_consumer.md`

Chosen review base: `1a790e985` (`[plan] Activate RISC-V prepared edge publication consumer plan`)

Base rationale: this commit activates idea 24 and creates the current `plan.md`/`todo.md` state. The later `3887f3980` inventory commit records Step 1 execution but does not reset the active source idea or rewrite the route, so the activation commit is the right active-idea checkpoint.

Commits since base: 2 committed changes through `HEAD` (`1462d41e5`), plus the current uncommitted `todo.md` metadata repair advancing the current step to Step 4 and requesting this review.

## Findings

No blocking findings.

Low: `todo.md` currently contains the review reminder line `你該做code review了` immediately after the Step 4 metadata. That is not an implementation or route blocker, and it is consistent with the current review trigger context, but the supervisor should remove or let the lifecycle hook retire it after this report is consumed so the scratchpad does not keep a stale always-on review prompt.

## Alignment Review

The committed implementation matches the idea 24 contract. The source idea asks for RISC-V to consume shared prepared `edge_publications` for a register-destination source-home family while preserving fail-closed behavior outside the scoped surface. The implementation adds a RISC-V-owned consumer in `src/backend/mir/riscv/codegen/emit.cpp:61` that queries `prepare::find_unique_indexed_prepared_edge_publication(...)` and only accepts available move publications with `Register` source and `Register` destination homes at `src/backend/mir/riscv/codegen/emit.cpp:92` through `src/backend/mir/riscv/codegen/emit.cpp:113`.

The route also keeps target-local emission policy in RISC-V. The emitted instruction text is formed as RISC-V `mv` syntax in `src/backend/mir/riscv/codegen/emit.cpp:113`, and the public RISC-V BIR entrypoint prepares semantic BIR and calls the RISC-V prepared-module emitter at `src/backend/backend.cpp:1348` through `src/backend/backend.cpp:1352`. The change does not move RISC-V register spelling, move syntax, or formatting into shared prepare.

I do not see testcase-overfit or local rediscovery. The implementation reads the shared lookup index instead of scanning predecessor or successor blocks. The focused test clears `publications_by_edge_destination` and expects `MissingPublication` at `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:142` through `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:146`, which directly proves the helper cannot recover the move from local fixture shape after shared authority is removed. Negative tests also preserve fail-closed behavior for missing lookups, missing publications, stack source homes, stack destinations, and non-move publications at `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:152` through `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp:207`.

The unimplemented homes are correctly represented as current Step 4 work rather than overclaimed support. `todo.md` says `StackSlot -> Register` and `RematerializableImmediate -> Register` are not implemented by this first packet, and it keeps `PointerBasePlusOffset -> Register` plus source-to-`StackSlot` destinations out of scope.

## Judgments

Idea-alignment judgment: `matches source idea`

Runbook-transcription judgment: `plan matches idea`

Route-alignment judgment: `on track`

Technical-debt judgment: `acceptable`

Validation sufficiency: `narrow proof sufficient`

Reviewer recommendation: `continue current route`

## Step 4 Recommendation

Proceed to Step 4 source-home broadening decision. No plan or todo rewrite is needed before that decision.

The next packet should decide whether to fold in `StackSlot -> Register` and/or `RematerializableImmediate -> Register` using the same shared lookup model and RISC-V-local emission policy. Keep `PointerBasePlusOffset -> Register` and source-to-`StackSlot` destinations fail closed unless a separate lifecycle decision changes scope.

Validation is sufficient for this checkpoint: `todo.md` records the delegated focused proof passing 5/5 selected tests, the matching regression guard passing against the focused before/after logs, and broader backend validation passing 163/163. The current `test_after.log` also shows the backend bucket at 163/163 with `backend_riscv_prepared_edge_publication` included.
