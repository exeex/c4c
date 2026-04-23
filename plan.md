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
save-clobber ownership, and related call-boundary obligations.

Primary target:

- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prepare.cpp`
- `src/backend/prealloc/prepared_printer.cpp`

Actions:

- strengthen `call_plans` publication for before-call and after-call
  obligations
- clarify caller-save, callee-save, and call-clobber authority where target
  backends still infer it
- cover scalar edge cases such as variadic, byval, indirect, memory-return, or
  nested dynamic-stack interactions when they expose missing prepared facts

Completion check:

- target backends can follow published scalar call-boundary plans directly
- prepared call records expose authoritative movement and clobber ownership

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
