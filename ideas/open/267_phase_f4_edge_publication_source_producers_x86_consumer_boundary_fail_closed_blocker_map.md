# 267 Phase F4 edge_publication_source_producers x86 consumer boundary fail-closed blocker map

## Goal

Map the x86 consumer boundary and fail-closed blockers for
`PreparedFunctionLookups::edge_publication_source_producers`.

This is analysis/blocker mapping only unless it names a real x86 consumer and a
supported fail-closed route for the producer/source relation.

## Why This Exists

The post-F3 gate kept `edge_publication_source_producers` blocked because the
row still lacks a concrete x86 consumer boundary that joins prepared
source-producer rows to the same Route 5/BIR source-producer identity and
rejects disagreement.

## In Scope

- Identify whether x86 has a real consumer of the source-producer relation.
- If a consumer exists, name the boundary where prepared and Route 5/BIR
  producer/source identity must agree.
- Map duplicate/conflict/mismatch/missing rows.
- Map prepared-only rows.
- Map fallback rows.
- Map `LoadLocal` memory-source rows.
- Map immediate-producer rows.
- Map policy-sensitive rows.
- Name required fail-closed behavior and x86/riscv evidence or explicit
  non-applicability for each row.

## Out Of Scope

- Lookup demotion, deletion, privatization, accessor wrapping, or adapter
  migration.
- Any source-producer implementation packet that lacks a named x86 consumer
  boundary.
- Moving target-owned publication/output, immediate encoding, storage,
  addressing, carrier/helper, formatting, wrapper, ABI, register, stack,
  instruction spelling, or emission policy into BIR.
- Broad prepared aggregate retirement.

## Completion Criteria

- The map either names a concrete x86 consumer boundary or records that no
  implementation packet is safe yet.
- The map covers duplicate/conflict/mismatch/missing, prepared-only, fallback,
  `LoadLocal` memory-source, immediate-producer, and policy-sensitive rows.
- Each row names the expected fail-closed behavior and preserved compatibility
  surfaces.
- Any later implementation idea is blocked unless this map identifies both a
  semantic authority and a supported consumer path.

## Reviewer Reject Signals

- The slice claims progress without naming a real x86 consumer boundary for the
  producer/source relation.
- The slice weakens unsupported expectations, helper/oracle statuses, fallback
  names, route-debug output, prepared-printer output, wrapper output, exact
  target output, or baseline behavior.
- The slice claims capability progress through expectation rewrites, helper
  renames, status/oracle relabeling, route-debug/printer output changes, or
  classification-only notes.
- The slice hides old public `edge_publication_source_producers` authority
  behind a renamed BIR field, route field, private accessor, adapter, wrapper,
  or compatibility helper without proving it is only a mirror.
- The slice migrates target-owned publication/output, immediate, storage,
  addressing, carrier/helper, formatting, emission, instruction spelling,
  wrapper, ABI, register, or stack policy into BIR.
- The slice proves one named fixture while duplicate/conflict/mismatch/missing,
  prepared-only, fallback, memory-source, immediate-producer, or
  policy-sensitive rows remain unexamined.
- The slice authorizes broad prepared retirement or lookup demotion.
