# F3 Terminal Emission Handoff Boundary

Contract-Status: converged planned contract under idea 732
Implementation-Status: partial external target assemblers/linkers exist; this
uniform verified-machine admission and terminal-output contract is absent

Phase: F3
Upstream: exact F2 verified-machine capability and borrowed machine graph
Downstream: external target assembler, object, and linker owners

## Purpose

F3 owns the terminal admission, routing, and disposition boundary from verified
machine records to assembly/object/relocation/link consumers. It does not take
semantic authority from those external domains.

## Owns

Exact F2 admission, stable record routing, opaque-inline-asm late-parser
handoff, output-request selection, terminal product manifesting, and
failure-atomic publication of requested outputs.

## Does Not Own

Machine construction/verification, allocation, spilling, copy/ABI/frame
repair, target instruction semantics, assembler grammar, encoding algorithms,
object-model semantics, relocation semantics, linker policy, or loader policy.

## Inputs

One exact F2 capability and graph, requested output mode, external assembler/
encoder/object/linker identities and versions, section/symbol/relocation facts,
and destination policy. Unverified graphs and detached facts are rejected.

## Input NodeKind/Tag Vocabulary

Exactly F2-verified ordinary/move/memory/frame machine records, opaque inline-
asm records, symbol/relocation-bearing records, and declared data/metadata
records. External consumers refine their own closed target vocabularies.

## Required Analyses and Products

F2 keys and readiness digest, target and output-mode keys, plus external
assembler/object/linker configuration must all match. F3 derives routing and
terminal manifests, not allocation, frame, machine-legality, or linker policy.

## Ordered Behavior

1. Freeze exact F2 capability, graph, output request, and consumer versions.
2. Route normal records to the target encoder and opaque inline-asm payloads
   unchanged to the designated late assembler parser.
3. Collect encoder bytes and exact symbol/section/relocation contributions into
   the external object model when object output is requested.
4. Serialize assembly text, encoded image, object file, and/or invoke the
   selected external linker only as requested.
5. Verify the terminal manifest and atomically publish all requested outputs,
   or publish none of this transaction's outputs.

## NodeKind/Tag Lowering Matrix

| F3 input/output subset | Disposition | External semantic owner | BIR-local obligation | Failure |
|---|---|---|---|---|
| verified ordinary/move/memory/frame record | route to printer/encoder | target assembler/encoder | preserve order, operands, forms, provenance | unhandled/changed record rejects |
| verified opaque inline-asm record | pass unchanged to designated late parser | target assembler parser | preserve bytes, bindings, clobber/effect envelope | early parse or payload drift rejects |
| verified symbol/relocation-bearing record | route contributions | target assembler and object/relocation owner | preserve exact intent and source record link | dropped/rewritten intent rejects |
| assembly-text request | publish requested text artifact | target printer/assembler grammar | manifest exact graph and consumer version | partial/untracked output rejects |
| encoded-image/object request | publish requested bytes/object and relocation set | target encoder and external [object boundary](../../mir/object/README.md) | manifest sections/symbols/relocations and hashes | inconsistency rejects |
| link request | hand exact object set/options to selected linker; publish linked output | target linker | manifest inputs, options, linker identity and result | linker failure publishes no claimed linked output |
| unknown, stale, unverified, omitted, or unsupported mode | reject | none | no fallback | `F3VocabularyInvalid` |

## Identity and Provenance

Terminal artifacts receive distinct artifact identities and content digests.
Machine and BIR IDs remain provenance references only. An assembly file,
object, image, and linked output are never interchangeable capabilities.

## Outputs

As requested: assembly text, encoded instruction/data image, section/symbol/
relocation-bearing object representation or serialized object file, linked
image/library, diagnostics, and one terminal manifest naming exact inputs,
consumer versions, options, artifacts, hashes, and downstream destination.

## Verification and Publication

The gate proves exact F2 identity, total routing, unchanged opaque transport,
consumer completion, artifact/manifest correspondence, and requested-output
completeness. A linker handoff is explicit; successful object emission alone
cannot claim a linked output.

## Analysis Preservation and Invalidation

F3 preserves no mutable compiler analysis. Any graph/capability, target,
consumer, option, object, relocation, destination, or artifact change
invalidates the manifest and every dependent publication.

## Failure and Diagnostics

Parser, encoder, relocation, object, I/O, linker, cancellation, or resource
failure is terminal for the requested transaction. F3 never backfills,
rewrites, expands, reallocates, spills, reschedules copies, or repairs frames.

## Adjacent-Stage Contract

F2 alone supplies verified-machine admission. The external [MIR boundary](../../mir/README.md),
target assemblers/encoders/linkers, and [object boundary](../../mir/object/README.md)
retain their domain authority; F3 owns only exact handoff and output accounting.

## Implementation State

AArch64, RISC-V, and x86 target trees contain varying printer, assembler,
encoder, object, and linker implementations. They are partial evidence, not
proof of this uniform F2-gated route or all terminal outputs.

## Proof Requirements

Prove exact F2 admission, total routing, designated late inline-asm parsing,
complete output ownership, exact relocation/object/link handoffs, artifact
manifest correspondence, fail-closed unknowns, and no earlier-phase repair.

## Open Questions

None for ownership. Supported output modes and target-specific consumer
coverage must be recorded by later implementation evidence, never inferred.
