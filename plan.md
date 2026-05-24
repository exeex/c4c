# Prealloc Call Plan Phase Split Runbook

Status: Active
Source Idea: ideas/open/prealloc-call-plan-phase-split.md

## Purpose

Separate durable call-plan subphases inside `src/backend/prealloc` while
preserving the current call ABI contract and backend behavior.

## Goal

Make `call_plans.cpp` smaller or better structured around stable direct-call,
indirect-call, preservation, clobber, boundary-effect, and memory-return
responsibilities without prematurely splitting the aggregate call contract.

## Core Rule

Treat call and return ABI planning as behavior-preserving prealloc structure
work. Do not change ABI rules, argument or result placement, clobber sets,
callee handling, preservation behavior, memory-return behavior, dump meaning,
or target policy ownership.

## Read First

- `ideas/open/prealloc-call-plan-phase-split.md`
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/call_plans.hpp`
- `src/backend/prealloc/calls.hpp`
- `src/backend/prealloc/formal_publications.cpp`
- `src/backend/prealloc/formal_publications.hpp`
- `src/backend/prealloc/prepared_printer/calls.cpp`

## Current Targets

- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/call_plans.hpp`
- `src/backend/prealloc/calls.hpp`
- `src/backend/prealloc/formal_publications.cpp`
- `src/backend/prealloc/formal_publications.hpp`
- `src/backend/prealloc/prepared_printer/calls.cpp`

## Non-Goals

- Do not change ABI policy, argument/result placement, register facts,
  clobbering, preservation, indirect callee handling, or memory-return
  behavior.
- Do not move ABI policy away from `TargetProfile`, target register profile
  facts, or the existing call contracts.
- Do not split `calls.hpp` into many small target-consumed headers unless usage
  proves a smaller independently consumed contract.
- Do not rewrite backend tests or dump expectations to mask behavior changes.
- Do not make prepared-printer label changes that drop fields or alter printed
  meaning.
- Do not add target-shaped shortcuts or named-case-only handling.

## Working Model

`call_plans.cpp` owns the construction of call and return ABI plans. The
source idea expects structure around durable subphases, not a semantic rewrite.
Use this working model while auditing and editing:

- argument planning: direct and indirect call argument placement and material
  needed to build call plans
- preservation planning: caller/callee preservation requirements and save/restore
  responsibilities
- clobber derivation: the call-side facts that describe registers or resources
  invalidated by a call
- boundary effects: side effects at call boundaries that later backend stages
  consume
- memory-return handling: the special result path for memory-return ABI cases
- aggregate contract: `calls.hpp` remains the public call-plan contract unless
  a smaller independently consumed boundary is proven
- prepared-printer mirror: `prepared_printer/calls.cpp` mirrors call-plan data
  for dumps and should follow real data-family changes only

## Execution Rules

- Start with an audit of helpers, data flow, includes, and prepared-printer
  mirrors before extracting code.
- Extract only boundaries that match durable call-planning responsibilities and
  keep helper inputs explicit.
- Prefer file-local or narrow `.cpp` structure first; introduce new declarations
  only when call sites prove the boundary needs to be public.
- Keep `calls.hpp` as the aggregate public contract unless the audit proves a
  smaller consumer-facing contract.
- Record deferred extraction candidates in `todo.md` instead of forcing broad
  movement into one packet.
- If prepared-printer grouping or labels are touched, they must mirror a real
  call-plan data-family change and preserve dump meaning.
- For code-changing packets, require:
  `bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'`
- Always run `git diff --check` before handing a packet back.

## Step 1: Audit The Call Plan Surface

Goal: identify the current call-plan helper families, public declarations,
data flow, and prepared-printer mirrors before extracting code.

Primary targets:
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/call_plans.hpp`
- `src/backend/prealloc/calls.hpp`
- `src/backend/prealloc/formal_publications.cpp`
- `src/backend/prealloc/formal_publications.hpp`
- `src/backend/prealloc/prepared_printer/calls.cpp`

Actions:
- Map major helpers and declarations to the working-model families.
- Identify direct-call and indirect-call argument planning code and whether
  each boundary has clear inputs.
- Identify preservation, clobber derivation, boundary-effect, and memory-return
  code paths.
- Record include or public-contract pressure, especially any evidence for or
  against splitting `calls.hpp`.
- Map prepared-printer helpers and labels to the call-plan data they mirror.
- Record stale wording, bridge/fallback/legacy naming, or broad mutable-state
  leakage candidates in `todo.md`.
- Do not edit implementation or header files in this step.

Completion check:
- `todo.md` contains the helper-family map, public-contract notes,
  printer-mirror map, extraction candidates, and explicit no-edit proof.
- No implementation, header, test, or source idea files are changed.

## Step 2: Extract Argument Planning Subphases

Goal: separate direct and indirect call argument planning where the audit proves
stable helper boundaries.

Primary targets:
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/call_plans.hpp` only if a declaration is proven needed

Actions:
- Extract direct-call argument planning helpers when inputs and outputs are
  explicit and behavior-preserving.
- Extract indirect-call argument planning helpers only when they are not
  entangled with unrelated call boundary effects.
- Keep helper names tied to call-planning responsibilities rather than testcase
  shapes.
- Avoid broad public declaration churn; keep helpers file-local when possible.
- Record any candidate that needs a larger call contract split in `todo.md`
  instead of changing `calls.hpp` prematurely.

Completion check:
- Argument planning structure is clearer or smaller by durable subphase.
- Backend tests pass with the delegated proof command.
- `todo.md` records changed files, extracted helper boundaries, and any
  deferred candidates.

## Step 3: Separate Preservation, Clobber, And Boundary Effects

Goal: isolate preservation, clobber derivation, and call-boundary effect logic
where clear helper APIs exist.

Primary targets:
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/call_plans.hpp` only if a declaration is proven needed
- `src/backend/prealloc/calls.hpp` only if usage proves a smaller aggregate
  contract boundary

Actions:
- Extract preservation helpers when they can own save/restore responsibility
  without leaking broad mutable planning state.
- Extract clobber derivation helpers when they preserve current target profile
  facts and call ABI behavior.
- Separate boundary-effect helpers only where the resulting inputs and outputs
  are explicit.
- Keep target policy anchored in `TargetProfile` and existing register profile
  facts.
- Stop and record a follow-up if the change requires splitting `calls.hpp` into
  many tiny target-consumed headers.

Completion check:
- Preservation, clobber, or boundary-effect code is clearer by durable
  responsibility without semantic change.
- Backend tests pass with the delegated proof command.
- `todo.md` explains why each extracted boundary is behavior-preserving.

## Step 4: Review Memory-Return And Formal Publication Boundaries

Goal: confirm memory-return handling and formal publication interactions remain
properly owned after the call-plan extractions.

Primary targets:
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/calls.hpp`
- `src/backend/prealloc/formal_publications.cpp`
- `src/backend/prealloc/formal_publications.hpp`

Actions:
- Inspect memory-return handling after earlier extractions and isolate it only
  if the boundary is clear.
- Verify formal publication code still consumes the same call and return facts
  without needing ABI-policy movement.
- Avoid moving formal publication behavior into call-plan helpers unless the
  dependency is already a call-plan responsibility.
- Record any remaining boundary pressure in `todo.md`.

Completion check:
- Memory-return and formal-publication boundaries are either clarified in code
  or documented as intentionally retained.
- Backend tests pass with the delegated proof command when code changed.
- `todo.md` records final ownership notes for memory returns and formal
  publications.

## Step 5: Align Prepared Printer Calls Mirror

Goal: keep call dump grouping aligned with real call-plan data-family changes
from earlier steps.

Primary target:
- `src/backend/prealloc/prepared_printer/calls.cpp`

Actions:
- Update prepared-printer helper names, grouping, or labels only when earlier
  steps changed the corresponding call-plan data family.
- Preserve printed fields and meaning.
- If no data-family naming or grouping changed, record that no printer edit was
  needed.

Completion check:
- Prepared-printer changes mirror actual call-plan structure changes, or
  `todo.md` records that no printer alignment was required.
- Backend tests pass with the delegated proof command when code changed.

## Step 6: Final Call Plan Boundary Review

Goal: decide whether the call-plan phase split source idea is complete and
ready for lifecycle closure.

Actions:
- Review the final diff against the source idea acceptance criteria and
  reviewer reject signals.
- Confirm `call_plans.cpp` is smaller or better structured by durable subphase.
- Confirm no ABI, clobber, preservation, indirect-callee, memory-return,
  formal-publication, or prepared dump semantics changed.
- Confirm `calls.hpp` remains aggregate unless a proven smaller public contract
  was created.
- Record final changed files, proof, deferred follow-up candidates, and closure
  recommendation in `todo.md`.

Completion check:
- `todo.md` contains the final boundary summary and proof status.
- The source idea can be closed without leaving important call-plan ownership
  decisions only in chat.
