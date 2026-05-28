# AArch64 Dispatch Prepared Publication Decomposition

## Goal

Split the stuck AArch64 instruction-dispatch prepared-publication route into
focused probes and backend seams before more implementation is attempted.

## Why This Exists

The active recovery route for
`ideas/open/58_aarch64_prepared_authority_regression_recovery.md` repaired or
advanced several real AArch64 prepared-authority seams, but
`backend_aarch64_instruction_dispatch` remained a monolithic route driver. Its
first bad assertion moved across unrelated surfaces:

- store-global publication ownership and accounting
- selected fused-compare materialization scratch choice
- call and outgoing stack argument materialization
- direct edge `LoadLocal` publication source-memory consumption
- GOT-required `LoadGlobal` materialization

The reviewer report in `review/step3_dispatch_route_review.md` allowed one
more narrow packet after the route drift warning. That packet exposed another
unrelated seam, so continuing to chase the next assertion would turn one broad
dispatch test into the execution strategy. This idea exists to replace that
route with focused, capability-oriented probes.

## In Scope

- Establish the current dirty-stack baseline for the dispatch/prepared
  publication family without reverting in-progress implementation edits.
- Inventory each separable prepared-publication seam reached by the monolithic
  dispatch route.
- Extract or identify focused backend probes, preferably under
  `tests/backend/case/`, so each probe owns one primary contract.
- Bind each focused probe to the backend surface that should own the semantic
  repair.
- Resume implementation only after a narrow probe and owner are selected.

## Seams To Split

- Store-global publication ownership: stack-homed selected values published
  for global stores must be accounted for by their store-owned prepared
  publication, not by generic unsupported suppression.
- Store-local publication ownership: selected local-store accounting must have
  an explicit prepared publication owner or a focused probe proving the current
  future-consumer lookup is safe.
- Fused-compare materialization: selected global-load or select-materialized
  operands must preserve operand order without forcing unrelated compare
  operands into fixed scratch registers.
- Call/outgoing stack arguments: prepared immediates, aggregates, and stack
  argument homes must materialize through the current call plan or move bundle
  without module-wide rederivation.
- Direct edge `LoadLocal` publication: matching edge publications must consume
  prepared source-memory authority and fail closed when that authority is
  absent.
- GOT-required `LoadGlobal` materialization: prepared global loads requiring
  GOT relocation must use GOT page/low12 materialization rather than treating
  the symbol as a direct page/low12 address.
- The separate `00196` runtime mismatch remains owned by the later
  `78730af2f` family from idea 58 unless decomposition proves it shares one of
  these dispatch seams.

## Expected Focused Probes

Prefer focused backend cases over more assertion-chasing inside
`backend_aarch64_instruction_dispatch`. Candidate probes should be one-contract
tests with names close to these capabilities:

- `tests/backend/case/aarch64_store_global_stack_publication.c`
- `tests/backend/case/aarch64_store_local_selected_publication.c`
- `tests/backend/case/aarch64_fused_compare_selected_operand_order.c`
- `tests/backend/case/aarch64_call_prepared_stack_arguments.c`
- `tests/backend/case/aarch64_edge_load_local_prepared_memory.c`
- `tests/backend/case/aarch64_got_load_global_prepared_memory.c`

If an existing focused case already covers a seam, reuse or extend it instead
of creating a duplicate. If no backend case can express a seam yet, record the
missing harness limitation in `todo.md` before implementation resumes.

## Out Of Scope

- Do not close or supersede idea 58; its four-test recovery goal remains open.
- Do not continue direct implementation against the next
  `backend_aarch64_instruction_dispatch` assertion before extracting or
  selecting a focused probe.
- Do not rewrite broad AArch64 dispatch, memory, calls, or publication helpers
  as part of decomposition unless a focused probe proves the owner boundary is
  wrong.
- Do not use testcase names, exact labels, literal stack offsets, or fixed
  temporary names as semantic authority.
- Do not weaken or mark unsupported any current failing test without explicit
  user approval.

## Acceptance Criteria

- The decomposition baseline records the current dirty implementation context,
  the focused proof result, and the current first bad assertion.
- Each visible seam is mapped to a focused probe or to a documented reason a
  probe cannot yet be extracted.
- At least one focused probe is selected as the next implementation owner, with
  a precise backend surface and proof command.
- The next implementation packet can proceed without using
  `backend_aarch64_instruction_dispatch` as the only route driver.
- Idea 58 remains open with its original four-test recovery criteria intact.

## Reviewer Reject Signals

- Reject changes that keep chasing successive
  `backend_aarch64_instruction_dispatch` assertions without first binding the
  changed behavior to a focused probe or documented seam owner.
- Reject focused probes that are merely smaller copies of the monolithic
  dispatch route and still mix store publication, compare materialization,
  calls, edge copies, and global-load relocation in one case.
- Reject testcase-shaped shortcuts, named-case checks, exact temporary-name
  matching, literal-label matching, or fixed stack-offset matching.
- Reject expectation rewrites, helper renames, todo-only classification, or
  plan-only churn claimed as backend capability progress.
- Reject downgrading supported tests to unsupported or weakening dispatch,
  backend route, or c-testsuite contracts without explicit user approval.
- Reject broad rewrites outside the selected seam for a packet unless the
  focused probe proves the shared prepared-publication owner is wrong.
- Reject retaining the old moving-first-bad-assertion route behind new helper
  names without reducing the dispatch/prepared-publication family into
  independently provable seams.
