# 268 Phase F4 PreparedBirModule liveness authority blocker follow-up

## Goal

Identify the authority, reader, compatibility surface, and fail-closed proof
needed before `PreparedBirModule::liveness` can move out of blocked public
prepared authority.

This is not a demotion, private-accessor, or implementation packet.

## Why This Exists

The post-F3 gate kept `PreparedBirModule::liveness` blocked. It needs one exact
identity-only reader, one semantic fact, the retained compatibility surface, and
full fail-closed proof before any later private-pass-context or demotion
proposal can be considered.

## In Scope

- Find the exact identity-only reader, if one exists.
- Name the single semantic liveness fact that should own the meaning.
- Identify the public prepared compatibility surface that must remain stable.
- Map absent/skipped liveness rows.
- Map stale rows.
- Map mismatch rows.
- Map duplicate/conflict rows.
- Map unsupported rows.
- Map fallback rows.
- Map derived printer and target behavior that must remain stable.

## Out Of Scope

- `PreparedBirModule::liveness` demotion, deletion, privatization, wrapper
  creation, or accessor migration.
- Broad `PreparedBirModule` retirement.
- Moving target register, stack, layout, storage, ABI, emission, wrapper,
  formatting, instruction spelling, or exact target-output policy into BIR.
- Rewriting printer, route-debug, helper/oracle/status, fallback, wrapper, or
  target expectations as a substitute for semantic proof.

## Completion Criteria

- The blocker map names the exact reader, or records that no exact
  identity-only reader exists yet.
- The map names the semantic fact and retained compatibility surface.
- The map covers absent/skipped, stale, mismatch, duplicate/conflict,
  unsupported, fallback, and derived printer/target behavior.
- The map does not authorize a private-pass-context or demotion packet until
  every fail-closed row has supporting evidence.

## Reviewer Reject Signals

- The slice claims liveness progress without naming one exact identity-only
  reader and one semantic fact.
- The slice weakens unsupported expectations, helper/oracle statuses, fallback
  names, route-debug output, prepared-printer output, wrapper output, exact
  target output, or baseline behavior.
- The slice claims capability progress through expectation rewrites, helper
  renames, status/oracle relabeling, route-debug/printer output changes, or
  classification-only notes.
- The slice hides old public `liveness` authority behind a renamed BIR field,
  route field, private accessor, adapter, wrapper, or compatibility helper
  without proving it is only a mirror.
- The slice migrates target-owned liveness-derived register, stack, layout,
  storage, ABI, formatting, emission, instruction spelling, wrapper, or exact
  output policy into BIR.
- The slice proves one named fixture while absent/skipped, stale, mismatch,
  duplicate/conflict, unsupported, fallback, or derived printer/target rows
  remain unexamined.
- The slice authorizes broad `PreparedBirModule` retirement or liveness
  demotion.
