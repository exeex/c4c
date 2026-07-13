# LIR-to-BIR Memory Migration Boundary

Status: reviewed import sub-boundary; implementation remains build-excluded.

This document is reviewed with `lir_to_bir/README.md` as a memory-import
sub-boundary within root stage `A1`. It does not create another root stage
between `A1` draft construction and `A2` Draft/Raw verification and
publication, and it does not authorize the quarantined source files in this
directory to enter the build.

## Input and identity

The only accepted input is structured LIR memory semantics: ordinary typed SSA
value identities, typed local/global/symbol identities, address spaces,
alignment, volatility and atomic ordering, object layout, sizes/counts, and
source ordering. Rendered addresses, initializer text, names, instruction
indices, cached provenance observations, and legacy route records cannot create
or repair identity.

The importer predeclares every referenced type, symbol, global, local, block,
instruction result, and object needed by a memory operation. Each exact source
identity maps once to the corresponding epoch/owner/slot/generation BIR ID.
Operands and results then use the same generic value graph as every other Raw
instruction. Migration-private maps and source IDs may survive only as
non-authoritative origin metadata and never escape publication.

## Output and ownership

This sub-boundary contributes ordinary typed memory nodes to the same private
`ModuleDraft` built by the parent importer. It may express semantic allocation,
address calculation, load/store, copy/move/set, lifetime, stack save/restore,
and atomic operations only when the closed Raw schema can represent all source
facts losslessly. Core owns the resulting nodes, IDs, order, definitions, uses,
and immutable semantic object-layout facts.

No standalone memory graph, partial module, provenance cache, prepared address,
or publication token is produced here. The later canonical memory pass owns
normalization; revision-bound analyses own derived provenance and memory-effect
facts. Target preparation and later allocation stages own target-dependent
realization. Raw/Canonical storage contains none of those derived products.

## Transaction and failure behavior

Memory import runs inside the parent module transaction. A missing structured
identity, unsupported source family, type/layout mismatch, unresolved fixup, or
builder error rejects the entire import. It must not emit an opaque placeholder,
parse renderer text, resurrect a legacy record, or publish the successfully
lowered prefix.

After all module families finish, `ModuleBuilder::finish()` yields one frozen,
unpublished `ModuleDraft`. Only the full Raw verifier may atomically consume
that exact revision through `verify_and_publish_raw(ModuleDraft&&)`. Any verifier
error or revision change returns the shared move-only `PublicationFailure` and
no `RawBir`; this memory sub-boundary has no bypass or independent verifier.

## Current bootstrap versus target design

The checked-in active importer currently implements only the bounded non-goto
inline-assembly carrier and does not import these memory families. Files in this
directory remain quarantined reference material and excluded from CMake.

The target design is the lossless mapping above. A memory family may enter the
active build only after its structured LIR carrier, closed Raw node, exact ID
mapping, module-transactional failure path, Raw rules, and focused interface
tests exist together. Legacy file presence is not evidence of support.

## Review disposition

- Root stage: `A1` LIR import and draft construction.
- Predecessor: typed LIR and the parent import inventory within `A1`.
- Output: memory entities inside the parent's single frozen `ModuleDraft`.
- Publication gate: the sole full Draft/Raw gate at `A2`, which publishes a
  verified `RawBir` or publishes nothing.
- Successor: root `B1` / `P01 legalize`; the later canonical memory pass
  normalizes, but cannot repair lossy or malformed import.
- Re-entry: forbidden for a published revision; import constructs a new draft.
- Stable identity: exact BIR IDs and generic SSA edges survive publication;
  importer maps and derived analyses do not.
- Legacy coverage: quarantined memory/address/provenance migration sources are
  reference inputs only; ownership remains mapped by `LEGACY_COVERAGE.md`.
