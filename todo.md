# Current Packet

Status: Active
Source Idea Path: ideas/open/732_bir_stage_document_convergence_umbrella.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define phase A and the deferred idea-734 handoff

## Just Finished

- Completed plan Step 1: revalidated the root Markdown inventory and recorded
  an exact one-owner classification and proposed A-through-F review order.
- The inventory is 44 BIR Markdown files: the root overview plus all 43 unique
  subordinate files linked by it. The root also directly links two external
  MIR boundary documents. All 45 root link targets exist; no current BIR
  Markdown file is missing from the index and no subordinate target is linked
  twice.
- The phase counts are A=3, B=16, C=9, D=5, E=4, F=0 internal BIR owners,
  and cross-cutting=7, totaling 44. Phase F has two directly linked external
  boundary documents.

### Exact internal owner classification

The row order is the proposed document-review order. Shared owners are listed
exactly once under cross-cutting and are revisited only at the named phase
gate; that does not transfer or duplicate their authority.

| Owner path | Phase | Kind | Ordered review point |
|---|---|---|---|
| `src/backend/bir/lir_to_bir/README.md` | A | stage | A1 typed-LIR import and private draft |
| `src/backend/bir/lir_to_bir/memory/README.md` | A | boundary | A1 memory-import sub-boundary, immediately after its parent |
| `src/backend/bir/core/README.md` | A | schema | A2 Raw schema before Draft/Raw gate |
| `src/backend/bir/passes/legalize/README.md` | B | pass | B1/P01 |
| `src/backend/bir/analysis/comparison/README.md` | B | analysis | immediately before earliest consumer B2/P02 |
| `src/backend/bir/passes/scalar/README.md` | B | pass | B2/P02 |
| `src/backend/bir/analysis/cfg/README.md` | B | analysis | immediately before B3/P03 planning |
| `src/backend/bir/passes/cfg/README.md` | B | pass | B3/P03 |
| `src/backend/bir/analysis/dominance/README.md` | B | analysis | after B3 CFG, immediately before B4/P04 |
| `src/backend/bir/analysis/publication/README.md` | B | analysis | before earliest normal B4/P04 value-flow consumer |
| `src/backend/bir/passes/ssa/README.md` | B | pass | B4/P04 |
| `src/backend/bir/analysis/memory_effects/README.md` | B | analysis | from Raw, immediately before earliest mutating consumer B5/P05 |
| `src/backend/bir/analysis/provenance/README.md` | B | analysis | after CFG/dominance, immediately before B5/P05 |
| `src/backend/bir/passes/memory/README.md` | B | pass | B5/P05 |
| `src/backend/bir/passes/aggregate/README.md` | B | pass | B6/P06 |
| `src/backend/bir/analysis/call_graph/README.md` | B | analysis | immediately before earliest normal B7/P07 helper/intrinsic consumer |
| `src/backend/bir/passes/intrinsics/README.md` | B | pass | B7/P07 |
| `src/backend/bir/passes/README.md` | B | support | B8 canonical pass orchestration/publication support |
| `src/backend/bir/pipeline/README.md` | B | boundary | B8 ordered pipeline and Canonical publication boundary |
| `src/backend/bir/target_layout/README.md` | C | schema | C2 after external C1 TargetProfile selection |
| `src/backend/bir/preparation/abi/README.md` | C | stage | C3 preparation 1 |
| `src/backend/bir/preparation/calls/README.md` | C | stage | C4 preparation 2 |
| `src/backend/bir/preparation/variadic/README.md` | C | stage | C5 preparation 3 |
| `src/backend/bir/preparation/address/README.md` | C | stage | C6 preparation 4 |
| `src/backend/bir/preparation/inline_asm/README.md` | C | stage | C7 preparation 5 |
| `src/backend/bir/preparation/runtime_helpers/README.md` | C | stage | C8 preparation 6 |
| `src/backend/bir/preparation/README.md` | C | support | C8 sequencing and atomic cumulative-bundle publication |
| `src/backend/bir/regalloc/constraints/README.md` | C | stage | C9 binding and later-revision projection authority |
| `src/backend/bir/passes/pseudo_lowering/README.md` | D | pass | D1 |
| `src/backend/bir/pseudo/README.md` | D | schema | D1 admitted pseudo schema, reviewed with its producer |
| `src/backend/bir/passes/call_lowering/README.md` | D | pass | D2 |
| `src/backend/bir/passes/target/README.md` | D | pass | D4 after shared D3 verifier touchpoint |
| `src/backend/bir/passes/out_of_ssa/README.md` | D | pass | D5 initial publication and post-E3 copy-resolution closure |
| `src/backend/bir/analysis/liveness/README.md` | E | analysis | E1 |
| `src/backend/bir/regalloc/README.md` | E | pass | E2, while also consuming the E1 product |
| `src/backend/bir/regalloc/spill_reload/README.md` | E | pass | E3 and retry to E1 |
| `src/backend/bir/allocated/README.md` | E | boundary | E4 final private closure and MIR-ready publication |
| `src/backend/bir/README.md` | cross-cutting | audit | normative index first; reconcile again in final umbrella audit |
| `src/backend/bir/analysis/README.md` | cross-cutting | support | shared analysis keys/cache/invalidation before the first phase analysis |
| `src/backend/bir/verify/README.md` | cross-cutting | boundary | shared profile owner at A2, B8, D3/D4/D5, and E4 gates |
| `src/backend/bir/diagnostics/README.md` | cross-cutting | support | after F, first root-declared cross-cutting review |
| `src/backend/bir/compatibility/README.md` | cross-cutting | support | after diagnostics |
| `src/backend/bir/LEGACY_COVERAGE.md` | cross-cutting | audit | after compatibility |
| `src/backend/bir/REVIEW_TEMPLATE.md` | cross-cutting | audit | final review/audit contract |

### Directly linked external boundary classification

| Owner path | Phase | Kind | Ordered review point |
|---|---|---|---|
| `src/backend/mir/README.md` | F | boundary | F1 strict one-pseudo-node/one-machine-record construction |
| `src/backend/mir/object/README.md` | F | boundary | F3 target-neutral object boundary after the unlinked F2/F3 authorities |

The exact phase flow is therefore A1 importer -> A1 memory sub-boundary -> A2
Raw schema/gate; B1 through B8 with each analysis immediately before its
earliest listed consumer; C1 external target selection -> C2 -> C3 -> C4 ->
C5 -> C6 -> C7 -> C8 -> C9; D1 -> D2 -> D3 shared gate -> D4 -> D5; E1 ->
E2 -> E3 -> E1 retry until stable -> D5 copy closure -> E4; then F1 -> F2 ->
F3. The shared verifier and analysis framework are inspected at their phase
touchpoints but remain cross-cutting owners. The four root-declared support/
audit documents are reviewed after F in their numbered root order.

### Seam-question ledger for child assignment

- Missing-index/missing-file: no current `src/backend/bir/**/*.md` file is
  absent from the root index and every internal link resolves. However, C1
  names only an unlinked "external target-profile authority", F2 names only an
  unlinked "external MIR/target verifier", and F3 names unlinked target
  assembler/linker authorities. Child C/F must decide whether those are
  intentionally non-Markdown external authorities or require explicit linked
  boundary owners/placeholders. Phase F must not silently expand from the two
  directly linked documents into every target-tree Markdown file.
- Duplicate authority: A1's parent importer and memory sub-boundary need an
  explicit ownership split; the latter's C++ migration units exist but the
  build deliberately excludes all nested `lir_to_bir/*.cpp`, so file presence
  is not implementation ownership. B8 must make the pass framework's
  invocation/publication rules and the pipeline's order/capability rules
  non-overlapping. Every phase must separate its local checks from the shared
  verifier's sole publication-gate authority.
- Duplicate authority: E1 is described in the stage table through the shared
  allocator while the analysis index separately names `analysis/liveness`.
  Child E must state that liveness owns immutable E1 facts and regalloc owns E2
  policy, or document another exact split. D5's post-E3 copy-resolution
  transaction and E4's enclosing publication transaction also need a single
  mutation/publication owner and an evidenced handoff, not two authorities.
- Stale status/format: all 44 BIR files lack the umbrella's exact metadata
  spine (`Contract-Status`, `Implementation-Status`, `Kind`, phase/applies-to,
  adjacency, owner path, reconciliation commit). Many subordinate files say
  "closed" or "converged" architecture contract; those are architecture
  review claims, not implementation-complete claims and must map to the new
  axes. The root still says idea 731 "remains open", while the active umbrella
  records it under `ideas/closed/`; that historical lifecycle statement is
  stale.
- Implementation truth: checked-in code currently proves a bounded bootstrap,
  not the documented A-F system: the core admits only `Opcode::InlineAsm`, the
  verifier exposes only FoundationRaw/TargetIndependentCanonical and ignores
  its profile argument, `canonicalize` verifies then relabels the same storage,
  and no P01-P07, C, D, E, or F implementation is present under the new BIR
  route. Nested importer-family sources exist but are build-excluded by
  `src/backend/CMakeLists.txt`; the top-level importer is a fail-closed bounded
  slice. Architecture prose and accepted review checkpoints cannot be used as
  evidence of stronger implementation status.
- Unproved adjacency: every A-F edge remains a contract assertion until the
  owning child supplies the umbrella-required input-coverage and output-
  handoff rows. Highest-risk seams are typed LIR -> ModuleDraft/RawBir (A1/A2),
  RawBir -> P01 and P07 -> CanonicalBir (A/B), CanonicalBir -> C1/C2 and C9 ->
  D1 (B/C/D), D2 -> shared D3 verifier -> D4, D4 -> D5, D5/E3 retry/copy
  closure -> E4, and MirReadyBirView/frame plan -> MIR/object (E/F). No current
  heading, status label, or green bounded-bootstrap test proves those seams.
- Phase-A source-gap watchout: present BIR documents repeatedly describe facts
  as current-LIR "source gaps". Step 2 must compare each claim against the
  complete current LIR surface and classify it as new-BIR receiving-container,
  importer-wiring, stale-documentation, or already-covered work. LIR remains
  immutable except for the umbrella's evidence-gated minimal inline-asm
  constraint carrier.

## Suggested Next

- Execute plan Step 2 as a docs-only phase-A intake packet: mechanically
  inventory every current typed-LIR variant and metadata family, then record
  the exhaustive Child-A matrix requirements and exact deferred idea-734
  boundary in `todo.md` without editing LIR, BIR docs, code, or idea 734.

## Watchouts

- The Step-1 classification is an assignment map, not acceptance of any
  document's architecture or implementation claims. Children must still prove
  every adjacency from actual producer and consumer contracts.
- Keep the shared verifier and analysis framework cross-cutting even when a
  phase reviews its relevant clauses; do not count them again as phase owners.
- LIR is complete for this route. Do not convert stale BIR `source gap` wording
  into LIR work. Keep ideas 734 and draft 733 inactive; this runbook authorizes
  no code.

## Proof

- Docs-only packet: no canonical regression log applies and `test_after.log`
  was not changed.
- Exact read-only inventory/link/classification proof command:

```bash
python3 - <<'PY'
from collections import Counter
from pathlib import Path
import re

repo = Path.cwd().resolve()
root = Path('src/backend/bir/README.md')
todo = Path('todo.md').read_text()
docs = sorted(Path('src/backend/bir').rglob('*.md'))
section = todo.split('### Exact internal owner classification', 1)[1].split(
    '### Directly linked external boundary classification', 1)[0]
rows = re.findall(
    r'^\| `(src/backend/bir/[^`]+\.md)` \| ([^|]+?) \| ([^|]+?) \|',
    section, re.M)
recorded = [Path(path) for path, _, _ in rows]
assert len(docs) == 44, len(docs)
assert len(recorded) == 44 and len(set(recorded)) == 44
assert set(recorded) == set(docs)

links = re.findall(r'\[[^]]+\]\(([^)#]+)(?:#[^)]*)?\)', root.read_text())
resolved = [(root.parent / target).resolve() for target in links]
internal = {path for path in resolved if path.is_relative_to((repo / 'src/backend/bir'))}
external = {path for path in resolved if not path.is_relative_to((repo / 'src/backend/bir'))}
assert len(links) == 45 and len(set(resolved)) == 45
assert all(path.exists() for path in resolved)
assert internal == {(repo / path).resolve() for path in docs if path != root}

external_section = todo.split(
    '### Directly linked external boundary classification', 1)[1].split(
    '### Seam-question ledger', 1)[0]
recorded_external = {
    (repo / path).resolve()
    for path in re.findall(r'^\| `(src/backend/mir/[^`]+\.md)` \|',
                           external_section, re.M)
}
assert external == recorded_external
counts = Counter(phase.strip() for _, phase, _ in rows)
assert counts == Counter({'B': 16, 'C': 9, 'cross-cutting': 7,
                          'D': 5, 'E': 4, 'A': 3})
print(f'PASS docs={len(docs)} root_links={len(links)} '
      f'internal_links={len(internal)} external_links={len(external)} '
      f'classification={len(recorded)} counts={dict(counts)}')
PY
```

- Result:

```text
PASS docs=44 root_links=45 internal_links=43 external_links=2 classification=44 counts={'A': 3, 'B': 16, 'C': 9, 'D': 5, 'E': 4, 'cross-cutting': 7}
```
