# LIR Universal Model And String Escape-Hatch Deletion Runbook

Status: Active
Source Idea: ideas/open/847_lir_universal_model_string_escape_hatch_deletion.md

## Purpose

Complete the terminal LIR universal-model deletion pass after accepted M1--M15 replacement gates, then hand valid LIR's final disposition to 797.

## Goal

Remove the remaining migrated universal LIR APIs, semantic string escape hatches, implicit string conversions, textual equality/classification paths, and expired adapters without creating a replacement compatibility model.

## Core Rule

Delete only surfaces whose native-authoritative replacements have accepted evidence; do not downgrade expectations, hide compatibility behind renamed helpers, or treat 797 as a catch-all owner for unfinished deletion work.

## Read First

- `ideas/open/847_lir_universal_model_string_escape_hatch_deletion.md`
- `ideas/closed/846_lir_family_overloaded_verifier_dispatch_printer.md`
- prior accepted deletion gates from ideas 838--846 when selecting each removal target
- current LIR model, verifier, printer, dispatcher, and adapter code before editing

## Current Targets

- universal `LirTypeRef` APIs, fields, factories, conversions, and adapters that are fully migrated
- semantic `runtime_text` use in valid LIR paths
- mutable `str()` or string-backed semantic escape hatches
- implicit string conversions and textual equality/classification paths used as semantic authority
- expired compatibility adapters left behind by the M1--M15 migrations
- final valid-LIR disposition notes required for handoff to 797

## Non-Goals

- Do not retain or introduce a universal compatibility substitute.
- Do not take ownership of 797's final coverage convergence.
- Do not route residual non-type strings owned by 812 or 813.
- Do not pull switch selector surfaces from ideas 821 or 822 into this deletion route.
- Do not convert unresolved selected semantic callers by expectation downgrade or named-case shortcut.

## Working Model

- 846 closed with native-authoritative verifier/printer consumer migrations accepted for selected integer, vector, aggregate, scalar boundary, and memory/index rendering gates.
- 847 owns the terminal deletion pass for remaining generic helpers, mutable semantic string escape hatches, implicit string conversions, and expired compatibility adapters.
- Any remaining selected semantic caller blocks deletion until it is classified under the accepted predecessor gates or routed to its proper owner.
- The final product is a complete valid-LIR disposition handoff to 797, not closure of 797 itself.

## Execution Rules

- Keep deletions narrow and mechanical after each target is proven migrated.
- Prefer compile failures and typed helper APIs as proof that universal compatibility cannot be used silently.
- Add nearby same-feature coverage when deleting a behaviorally visible compatibility path.
- For every code-changing step, run a fresh build plus the narrow focused proof; escalate to broader regression/coverage proof at the final deletion checkpoint.
- Record only executor packet progress in `todo.md`; do not edit the source idea unless durable scope or closure state changes.

## Steps

### Step 1: Audit deletion readiness and remaining target inventory

Goal: establish the exact removable surfaces and blockers before touching code.

Primary targets:
- universal `LirTypeRef`
- `runtime_text`
- mutable `str()`
- implicit string conversions
- textual equality/classification helpers
- expired adapters

Actions:
- inspect the current LIR code for remaining universal fields, factories, conversions, adapters, and semantic string authority
- map each candidate deletion target to accepted predecessor evidence from 838--846 or to an out-of-scope owner
- confirm switch selector surfaces stay outside this route under 821/822
- identify the first narrow deletion packet with no selected semantic callers

Completion check:
- `todo.md` names the first concrete deletion packet and records any blocked candidates with their owning idea or predecessor gate.

### Step 2: Delete migrated universal type-reference compatibility

Goal: remove fully migrated `LirTypeRef` compatibility surfaces without replacing them with a generic bag.

Actions:
- remove migrated universal fields, factories, conversions, and adapters
- update direct users to native family-specific APIs already accepted by predecessor work
- keep legacy or out-of-scope callers routed outside 847 instead of broadening this plan
- add or update nearby coverage only where deletion changes visible behavior or diagnostics

Completion check:
- fresh build and focused LIR tests pass
- searches show no remaining migrated universal `LirTypeRef` compatibility surfaces in valid LIR paths

### Step 3: Delete semantic string escape hatches

Goal: remove semantic `runtime_text`, mutable `str()`, implicit string conversions, and textual equality/classification as valid-LIR authority.

Actions:
- replace remaining valid-LIR semantic string authority with typed/native facts
- delete mutable or implicit string APIs once no selected callers remain
- preserve non-type string routes owned by 812/813 and switch selector routes owned by 821/822
- strengthen tests around the same feature family when a string fallback previously accepted invalid authority

Completion check:
- fresh build and focused LIR verifier/printer/parser tests pass
- searches show no semantic `runtime_text`, mutable `str()`, implicit conversion, or textual classification path remains as valid-LIR authority

### Step 4: Remove expired adapters and prove compile-time separation

Goal: ensure deleted compatibility cannot be re-entered through leftover adapters or dispatcher fallbacks.

Actions:
- remove expired adapters made obsolete by M1--M15
- inspect receiver, dispatcher, verifier, printer, and construction paths for universal fallbacks
- resolve compile errors by using native-specific facts, not by adding new compatibility shims
- preserve any residual owner-specific strings by documenting their owner in `todo.md`

Completion check:
- fresh full build passes
- targeted searches show no universal fields, factories, conversions, semantic runtime text, or expired adapters in valid LIR authority paths

### Step 5: Handoff final valid-LIR disposition to 797

Goal: deliver complete disposition evidence for 797 without claiming 797 is complete.

Actions:
- summarize deleted surfaces, residual out-of-scope owners, and proof results for the valid-LIR disposition
- identify any remaining coverage convergence work that belongs to 797
- ensure the handoff is evidence-backed and does not hide unfinished 847 deletion work

Completion check:
- broad regression/coverage proof selected by the supervisor passes
- 797 handoff evidence names deleted surfaces, accepted proof, and residual owners
- no 847 acceptance reject signal remains true
