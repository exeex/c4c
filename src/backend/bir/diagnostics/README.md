# Stage Diagnostics and Rendering

Status: design contract; implementation has not started.

Diagnostics and rendering are read-only, non-authoritative observations of an
already existing stage product. They never participate in verification,
canonicalization, preparation, pseudo lowering, allocation, MIR construction,
or emission. A diagnostic message, rendered dump, audit event, snapshot,
counter, or cache hit is not an input fact for any of those operations.

## Borrow and revision contract

An observation begins by borrowing one public immutable view. The supported
subjects are `RawBir`, `CanonicalBir`, cumulative preparation products,
`PseudoBir` (including the fully reverified D4 and post-D5 revisions),
`AllocatedBir`, `PreparedBir`, `MirReadyBirView`, and a published MIR view. The
observation records the subject's complete revision key and relevant target,
layout, preparation, constraint, pseudo-schema, allocation, and product
fingerprints before reading any entity.

The borrow cannot outlive its owner, retain mutable storage, clone an
instruction graph, refresh a stale key, or resolve an ID through another
revision. A change observed before completion cancels the observation and
returns a structured `ObservationRevisionMismatch`; it does not retry against
the newer subject. `PreparedBir` is displayed only as readiness metadata and
`MirReadyBirView` borrows the same immutable revision owned by `AllocatedBir`.

Renderers receive only the public view plus optional presentation settings.
They cannot construct stage products, invoke a publication gate, run a pass or
verifier, mutate an analysis cache, select a target route, assign a home,
insert `Spill`/`Reload`, choose a frame offset, or request MIR fallback.

## Structured diagnostics

The producing verifier, pass, planner, allocator, or MIR operation owns the
structured record. Every record contains a stable category/rule identifier,
severity, exact stage and revision key, typed entity identity when available,
source/provenance location when available, and ordered typed arguments. The
producer fixes diagnostic order at its transaction boundary. Presentation may
filter or format that order but may not add, remove, promote, demote, merge, or
reinterpret a fact used to determine success.

Text is presentation only. Names, labels, pointer values, slot indices,
pretty-printed operands, LLVM-compatible inline-asm spelling, and complete
rendered messages are never semantic identity. Tests of compiler behavior
assert structured rules and entity keys; snapshot tests may separately assert
text formatting.

## Deterministic rendering

A renderer walks the subject's normative public order. Unordered diagnostic or
audit attachments are sorted by their declared stable keys. Formatting options
cover whitespace, numeric radix, optional provenance, and redaction only; they
cannot change traversal, synthesize missing entities, or select a different
stage product. Inline-asm source and constraint text is escaped byte-for-byte.
LLVM-compatible spelling is explicitly labeled as a non-authoritative display
field and never overwrites the original payload.

A cache key is the renderer schema version, all presentation options, and the
complete borrowed subject key. Cached output contains bytes only, holds no
stage borrow, and is discarded on any key mismatch. Debug-only publication
views and target-machine printers obey the same rule.

## Failure and ownership boundaries

Observation can fail for a stale/foreign ID, expired borrow, revision mismatch,
invalid presentation option, output/resource limit, or I/O cancellation. Such
failure leaves the observed product unchanged and cannot change compilation
success. If rendering encounters a fact absent from the public view, it emits
an observation error rather than consulting legacy state or deriving a
replacement.

Semantic invariants and failure classification remain with the stage verifier
or producer. `diagnostics/` owns only structured presentation schemas,
deterministic formatters, and observation errors. Legacy BIR/printer paths,
prepared-printer family files, target-machine printers, names, label identity,
lookup-agreement displays, and debug publication views are coverage inputs;
their retained behavior is dispositioned in `LEGACY_COVERAGE.md`.

## Required proof

- Mutate or replace a candidate after taking an observation key and prove the
  observer rejects the stale borrow without emitting output for the new key.
- Render the same immutable subject under different worker schedules and prove
  byte-identical output and structured ordering.
- Feed rendered text, names, pointer/index observations, and a cached dump back
  toward every stage API and prove that no such API accepts them.
- Exercise diagnostic truncation, formatter failure, and I/O cancellation and
  prove the underlying transaction result and product fingerprints are
  unchanged.
