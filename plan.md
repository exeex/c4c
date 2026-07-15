# LIR-To-New-BIR Container And Import Completeness Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md
Resumed from: closed 752 local-object pointer-authority handoff.

## Purpose

Continue the established typed Raw-BIR import route at its first receiver row
authorized by closed 752, without repeating accepted PHI receipt work.

## Goal

Receive exactly one selected hoisted local alloca row using structured,
current-function authority.

## Core Rule

Only typed `LirAllocaOp.result` and `local_object_authority` facts may drive
receipt. Displayed local names and `%t` spellings are rendering mirrors, never
semantic input. A malformed row rejects transactionally before publication.

## Historical Progress

Steps 1 through 7.25 are accepted historical work, including the completed
parallel-edge PHI receipt in `006d79aaf` and `7dc03f23a`; do not repeat them.
Closed 752 (`ca26a8242`, `b200ac033`, `0e8093025`) supplies the next receiver
authority, with fresh full CTest 3037/3037 acceptance evidence.

## Current Scope

- receive only the selected hoisted `LirAllocaOp` row from its `result` and
  `local_object_authority` fields: `pointer_definition`, `object`, `owner`,
  `pointer_type`, `pointee_type`, and `live`;
- add the minimum Raw-BIR container/importer/reachable-verifier and
  transactional positive/negative proof for that one row;
- require the alloca-result binding, current-function pointer definition,
  alloca pointee agreement, and repeated-record canonicality already verified
  by the producer.

## Non-Goals

- local load/store/GEP, VLA stack-save/restore, and named/local-temporary
  variants beyond this selected alloca row;
- memory/va, aggregate/vector, body-parameter, module/type/global/metadata,
  PHI/CFG, target lowering, MIR, emission, and every other unreceived family;
- recovery from local display spelling, `%t`, printer output, or text.

## Ordered Steps

### Step 7.26 - Receive selected hoisted alloca authority

Goal: import precisely the 752-authorized selected hoisted `LirAllocaOp` row
into a typed Raw-BIR destination.

Actions:

- map only the alloca `result` and the six typed local-object authority fields
  into the existing-or-minimally-added target-independent Raw-BIR receiving
  container;
- validate presence, alloca-result binding, current-function ownership,
  pointer/object/type coherence, liveness, and canonical repeated authority;
- preserve transactional rejection for missing, invalid, foreign,
  pointer/object/type-mismatched, dead, or disagreeing authority;
- add nearby positive and malformed-authority coverage, then run a fresh build
  and narrow receiver proof before the supervisor selects broader acceptance
  validation.

Completion check: exactly this alloca row imports and verifies without any
presentation-derived recovery; no later local/VLA or unrelated family is
received.
