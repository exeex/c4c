# Prepared Frame-Stack-Call Authority Completion For Target Backends

Status: Active
Source Idea: ideas/open/88_prepared_frame_stack_call_authority_completion_for_target_backends.md
Activated from: ideas/open/88_prepared_frame_stack_call_authority_completion_for_target_backends.md
Supersedes: ideas/open/86_full_x86_backend_contract_first_replan.md

## Purpose

Complete target-independent prepared authority for frame, stack, and call
behavior so later backend stages consume published plans instead of rebuilding
ABI, stack, or save/restore policy locally.

## Goal

Reach a state where:

- frame layout ownership is published through prepared plans
- dynamic stack behavior is explicit and durable
- call-boundary argument, result, and clobber obligations are emitted as
  authoritative prepared facts
- target backends can trust prepared frame/stack/call plans directly

## Core Rules

- do not push stack or call policy back into target emitters once prealloc/BIR
  can publish it truthfully
- prefer target-independent contract fields and helpers over x86-local
  reconstruction
- keep scalar frame/stack/call authority separate from grouped-register work
- add proof whenever a prepared contract becomes more explicit

## Read First

- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prepare.cpp`
- `src/backend/prealloc/prepared_printer.cpp`
- `src/backend/mir/x86/`
- `tests/backend/`

## Current Targets / Scope

- `frame_plan`
- `dynamic_stack_plan`
- `call_plans`
- `storage_plans`
- prepared dumps and backend/prealloc tests covering call boundaries

## Non-Goals

- grouped-register bank/span/storage authority
- target-specific instruction spelling once truthful prepared plans exist
- frontend lowering work unless the first bad fact is still upstream of BIR
- broad x86 behavior restoration unrelated to frame/stack/call contracts

## Working Model

- BIR and prealloc should own final frame, stack, and call decisions when the
  required semantic facts are already known
- prepared data should publish executable authority for target backends
- target backends should consume prepared plans rather than recomputing ABI or
  frame policy from scattered hints

## Execution Rules

- if a backend still reconstructs call or frame policy locally, identify the
  missing prepared fact and publish it instead of adding another local helper
- keep fixed-frame and dynamic-stack coexistence explicit when both apply
- make before-call and after-call move obligations readable in dumps and tests
- separate scalar authority completion from any future grouped-register
  extension

## Step 1: Contract Surface Audit

Goal: inventory what prepared frame, stack, and call authority already exists,
what target backends still reconstruct locally, and which gaps belong to this
idea rather than grouped-register follow-on work.

Primary target:

- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prepare.cpp`
- `src/backend/prealloc/prepared_printer.cpp`
- relevant target backend consumers under `src/backend/mir/`

Actions:

- inventory current `frame_plan`, `dynamic_stack_plan`, `call_plans`, and
  related storage publication
- identify local backend reconstruction still required for scalar
  frame/stack/call behavior
- distinguish scalar contract gaps from grouped-register-only needs
- tighten dumps first if the missing authority is hard to inspect

Completion check:

- the active contract gaps are listed concretely
- the first implementation packet has a bounded prepared-surface target

## Step 2: Frame And Stack Authority Completion

Goal: publish durable prepared authority for fixed-frame and dynamic-stack
behavior so target backends stop reopening frame-layout policy locally.

Primary target:

- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prepare.cpp`
- `src/backend/prealloc/prepared_printer.cpp`

Actions:

- strengthen frame and dynamic-stack publication where fixed-frame anchoring or
  coexistence is currently implicit
- make storage and stack-region ownership explicit when prepared facts already
  decide them
- keep new contract fields target-independent and readable in dumps

Completion check:

- prepared frame/stack plans explain owned scalar cases without target-local
  policy recovery
- dumps expose the new authority clearly enough for review

## Step 3: Call Boundary Authority Completion

Goal: publish authoritative prepared call plans for argument/result movement,
save-clobber ownership, and related call-boundary obligations without leaving
target backends to recover scalar call semantics from raw BIR.

Primary target:

- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prepare.cpp`
- `src/backend/prealloc/prepared_printer.cpp`

Actions:

- execute the numbered substeps below in order
- keep each packet focused on one missing prepared call-boundary fact family
- treat grouped-register or bank/span issues as out of scope for this step

Completion check:

- all Step 3 substeps are complete
- target backends can follow published scalar call-boundary plans directly
- prepared call records expose authoritative movement and clobber ownership

### Step 3.1: Argument And Result Source Authority

Goal: make prepared call plans publish authoritative scalar argument and result
source shapes so consumers stop recovering register, stack, symbol, or
computed-address meaning from raw BIR.

Primary target:

- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prepare.cpp`
- `src/backend/prealloc/prepared_printer.cpp`

Actions:

- strengthen `call_plans` publication for before-call and after-call movement
  obligations
- preserve source-shape authority when transport storage differs from the
  semantic call operand origin
- cover remaining scalar argument/result forms such as stack-passed, indirect,
  byval, memory-return, or aggregate-adjacent scalar cases when prepared facts
  already know the answer

Execution note:

- Step 3.1 now runs through the numbered substeps below; prior Step 3.1
  packets already covered the argument-side publication route, so remaining
  execution should continue at Step 3.1.3 unless a new gap proves one of the
  earlier substeps incomplete.

#### Step 3.1.1: Argument Source-Shape Publication

Goal: publish direct scalar argument source shapes for immediate, symbol,
stack-slot, and memory-return-adjacent cases so prepared call plans expose how
each argument is formed without backend-side raw-BIR recovery.

Primary target:

- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prepare.cpp`
- `src/backend/prealloc/prepared_printer.cpp`
- `tests/backend/`

Actions:

- keep literal, symbol-address, frame-slot, byval, and memory-return-adjacent
  scalar argument forms explicit in `PreparedCallArgumentPlan`
- ensure prepared dumps and focused backend/prealloc tests expose those source
  shapes directly
- keep transport storage and semantic source shape separate when both matter

Completion check:

- prepared call plans publish the covered scalar argument source shapes
  directly
- dumps and focused tests make those shapes reviewable without correlating
  against raw BIR

#### Step 3.1.2: Argument Identity Authority For Indirect And Computed Sources

Goal: publish the missing scalar identities behind indirect callees and
computed-address argument bases so consumers do not recover ownership from
storage plans, move bundles, or value-location correlation.

Primary target:

- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prepare.cpp`
- `src/backend/prealloc/prepared_printer.cpp`
- `tests/backend/`

Actions:

- keep indirect-callee value identity explicit in prepared call plans
- publish computed-address base scalar identity alongside shape and byte-delta
  authority
- prove those identities through prepared dumps and focused call-contract tests

Completion check:

- prepared call plans expose the scalar owner for indirect-callee and
  computed-address argument cases directly
- backend consumers do not need storage-plan or regalloc correlation to
  recover those identities

#### Step 3.1.3: Result Source Authority Completion

Goal: publish any remaining scalar result-side source shape or identity facts
so prepared call plans describe where a call result semantically comes from
even when the published destination home differs from the ABI carrier.

Primary target:

- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prepare.cpp`
- `src/backend/prealloc/prepared_printer.cpp`
- `tests/backend/`

Actions:

- audit `PreparedCallResultPlan` for result-side facts that still require
  consumers to infer semantic source identity from move bundles or destination
  storage
- publish the next missing scalar result source-shape or identity field
  directly in prepared call plans
- extend prepared dumps and focused backend/prealloc tests to prove the new
  result authority explicitly

Completion check:

- prepared call plans publish the remaining scalar result-side source facts
  directly
- target consumers no longer need indirect correlation to understand covered
  scalar result movement

Completion check:

- prepared call plans publish scalar argument/result source authority directly
- prepared dumps expose those source shapes clearly enough for backend review
- target consumers no longer need raw-BIR recovery for covered scalar movement

### Step 3.2: Save, Clobber, And Preservation Authority

Goal: publish caller-save, callee-save, and call-clobber ownership as prepared
authority so backend consumers do not reconstruct preservation policy locally.

Primary target:

- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prepare.cpp`
- `src/backend/prealloc/prepared_printer.cpp`

Actions:

- clarify which values must be preserved across the call boundary and why
- publish call-clobber and save/restore ownership in target-independent terms
- make the prepared printer expose preservation authority in a reviewable form

Completion check:

- prepared call plans state scalar preservation and clobber authority directly
- backend consumers can follow published save/clobber facts without ABI
  re-derivation

### Step 3.3: Edge-Case Integration At Call Boundaries

Goal: finish the remaining scalar call-boundary cases where variadic behavior,
nested dynamic-stack interactions, or other edge conditions still hide missing
prepared facts.

Primary target:

- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prepare.cpp`
- `src/backend/prealloc/prepared_printer.cpp`
- `tests/backend/`

Actions:

- cover variadic and nested dynamic-stack interactions when they expose a real
  prepared-contract gap
- tighten tests and dumps around any remaining scalar call-boundary edge case
  added in this step
- stop and split into a new idea instead of absorbing grouped-register or
  broader backend reconstruction work

Completion check:

- no known scalar call-boundary edge case in scope still depends on hidden
  target-local reconstruction
- Step 4 can focus on proof and consumer confirmation rather than new contract
  discovery

## Step 4: Proof And Consumer Confirmation

Goal: lock the contract boundary into tests and confirm downstream consumers
use the new authority without hidden local reconstruction.

Primary target:

- `tests/backend/`
- `src/backend/prealloc/prepared_printer.cpp`
- relevant backend consumers under `src/backend/mir/`

Actions:

- add or tighten BIR/prepared/backend tests for frame, dynamic-stack, and
  call-boundary authority
- extend dump assertions so new prepared facts are stable and reviewable
- confirm target backend consumers read the published plans instead of
  rebuilding scalar policy locally

Completion check:

- owned backend/prealloc proof stays green with stronger prepared authority
- no surviving scalar frame/stack/call reconstruction remains hidden in target
  consumers for the covered cases
