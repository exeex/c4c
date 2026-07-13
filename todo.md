# Current Packet

Status: Active
Source Idea Path: ideas/open/737_bir_phase_c_preparation_document_convergence.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Converge C7 inline-assembly target vocabulary

## Just Finished

- Completed the authorized coordinated repair and plan Step 6 across
  `src/backend/bir/analysis/provenance/README.md` and
  `src/backend/bir/preparation/address/README.md`. The plan-owner's preexisting
  `plan.md` change remains untouched and records the same narrow authorization.
- Repaired Provenance stage applicability without weakening B4. Exact B4
  `SsaCanonical` remains the input for B5/P05, the earliest mutation consumer;
  exact verified later target-independent semantic checkpoints, including
  immutable B8 `CanonicalBir`, are now admitted for read-only queries such as
  C6.
- Required every later Provenance query to use exact-current same-revision
  schema-1 `Cfg`, `Dominance(Dominators)`, `PublicationValueFlow` and
  `MemoryEffects` dependency keys. B4 dependencies cannot be reused at B8, no
  dependency may be silently refreshed beneath an old request, and a result
  cannot be retagged under a later checkpoint.
- Preserved the analysis domain and semantics: `CanonicalSemantic`, canonical
  empty options, target key `None`, empty preparation, stable IDs/paths/parallel
  `EdgeKey`s, deterministic fixed point, Known/Absent/stable-reason Unknown
  versus malformed failure, atomic publication, stale rejection and no handle
  rebinding/recomputation.
- Added the exact C6 adjacency to Provenance. After B1-B8 mutations C6 requests
  a fresh B8 result and complete same-B8 dependency closure; equal-looking B4
  facts never establish freshness. Provenance remains implementation-absent
  with no descriptor, algorithm, schema, dependency route, validator, build
  target or runtime proof.
- Removed the C6 blocker metadata, failure and open-question clauses after the
  shared seam became inhabitable. The thirteen-row input matrix now accepts
  exact schema-1 B8 Provenance and rejects unknown, duplicate, schema/domain/
  stage-mismatched or stale handles without a special blocked state.
- Re-audited the fourteen C6 output families, eleven exact key axes and 37
  address forms covering global/local/TLS/string/function/label/block,
  relocation, GEP/path, dynamic/call/variadic objects, Known/Absent/Unknown and
  malformed/forbidden-inference cases. One immutable exact-key `AddressPlan`
  now publishes atomically or nothing.
- Preserved C6 requirements-only authority. Semantic base/index/path/scale/
  displacement/relocation domains and `RequiresD4Expansion` select no address
  mode, opcode/sequence, concrete register/offset/frame placement, assignment,
  helper, scratch, encoding or MIR record.
- Proved the exact downstream boundaries: C7 accepts only the complete C6
  fingerprint; D4 alone performs bounded target expansion; E4 alone fixes
  placements/displacements and materializes frame actions; F1 is apply-only
  over exact `MirReadyBirView` products and cannot expand, place or repair.
- Reconciled C6 implementation truth to `absent`: its directory contains only
  the README and no plan/key/requirements, registry, exact-current analysis
  route, transaction, cache, C7 handoff or D4/E4/F1 path exists. Legacy address
  and relocation code remains migration evidence only.
- Re-audited the authorized combined scope and `git diff --check`. The worktree
  contains only the plan-owner's preexisting `plan.md` authorization plus the
  two coordinated documents and canonical `todo.md`; no code, tests, build,
  logs or source idea changed.

## Suggested Next

- Execute plan Step 7 against
  `src/backend/bir/preparation/inline_asm/README.md`: converge immutable target
  vocabulary/context/eligibility/clobber tables while preserving opaque
  template/constraint bytes, ordinary values and C9's sole parsing/binding
  authority.

## Watchouts

- C7 consumes the complete same-key C3-C6 products but produces target tables
  only. It must not inspect a particular instruction's constraint order or
  attach meaning, ties, early-clobber exclusions or clobber units to operands.
- Keep assembly template and constraint bytes opaque and unchanged. Names,
  text renderings or placeholder positions cannot become semantic identity.
- Enumerate supported and absent vocabulary, context, eligibility and clobber
  domains explicitly; absence fails later binding rather than becoming a
  generic-register fallback.
- C9 alone parses/types/binds descriptions and projects constraints across
  later revisions. C7 selects no register, reservation, assignment, home,
  spill, frame fact, opcode, instruction or encoding.
- Provenance and C1-C6 remain documentation contracts without callable success
  paths; do not infer implementation from legacy inline-assembly helpers.

## Proof

- Documentation-only packet. The delegated scope forbids code, test, build and
  log edits; no canonical regression log was created or modified.
- Exact authorized scope, Provenance B4+B8 applicability/dependencies, C6 seam,
  matrices/forms/keys, authority/failure, implementation truth, diff check and
  Step-7 advance proof:

```bash
python3 - <<'PY'
from pathlib import Path
import re
import subprocess

provenance_path = Path('src/backend/bir/analysis/provenance/README.md')
provenance = provenance_path.read_text()
provenance_flat = ' '.join(provenance.split())
provenance_metadata = '\n'.join(provenance.splitlines()[:11])

for item in (
    'Contract-Status: under-review',
    'Implementation-Status: absent',
    'Kind: analysis',
    'Applies-To: B5 / P05 earliest consumer plus exact verified later target-independent semantic checkpoints, including B8, for read-only address queries',
    'Upstream: exact immutable verified B4-or-later semantic function plus exact-current same-revision CFG, dominance, value-flow and effects',
    'Downstream: B5 / P05 memory/address normalization and C6/later read-only planning',
    'Owner-Path: `src/backend/bir/analysis/provenance/README.md`',
    'Last-Reconciled-Commit: `5e7909f3e`',
):
    assert item in provenance_metadata, item

provenance_core = ('## Purpose', '## Owns', '## Does Not Own', '## Inputs',
                   '## Outputs', '## Adjacent-Stage Contract')
positions = [provenance.index(heading) for heading in provenance_core]
assert positions == sorted(positions)

def rows(text, heading, next_heading):
    body = text.split(heading, 1)[1].split(next_heading, 1)[0]
    return [line for line in body.splitlines() if line.startswith('|')][2:]

assert len(rows(provenance, '### Exact descriptor and input-key matrix',
                'The result key binds')) == 10
assert len(rows(provenance,
                '### Exhaustive provenance result-family matrix',
                '`Known`, `Absent`')) == 11

for item in (
    'B4 `SsaCanonical` checkpoint used by P05',
    'immutable B8 `CanonicalBir`',
    'exact-current for that same module/function revision and checkpoint',
    'AnalysisId::Cfg', 'AnalysisId::Dominance(Dominators)',
    'AnalysisId::PublicationValueFlow', 'AnalysisId::MemoryEffects',
    'B4 reuse at B8', 'cannot silently refresh one dependency',
    'canonical empty options; target key `None`; preparation empty',
    '`Known`, `Absent` and stable-reason `Unknown`',
    'Malformed core semantics fail',
    'old handles never rebind or recompute',
    'P05 memory', 'earliest mutation consumer',
    'C6 address preparation', 'read-only consumer',
    'Implementation is absent',
):
    assert item in provenance_flat, item

for link in re.findall(r'\[[^]]+\]\(([^)]+)\)', provenance):
    assert (provenance_path.parent / link).resolve().exists(), link

address_path = Path('src/backend/bir/preparation/address/README.md')
address = address_path.read_text()
address_flat = ' '.join(address.split())
address_metadata = '\n'.join(address.splitlines()[:11])
for item in (
    'Contract-Status: under-review',
    'Implementation-Status: absent',
    'Kind: product',
    'Phase-ID: C6',
    'Upstream: exact verifier-bound Canonical/C1 input, matching C2 `VerifiedTargetLayout`, exact C3 `AbiPlan`, exact C4 `CallPlan`, exact C5 `VariadicPlan`, and exact-current `Provenance`',
    'Downstream: one immutable exact-key `AddressPlan` consumed first by C7 inline-assembly target preparation and later by D1/D4/E4/F1',
    'Owner-Path: `src/backend/bir/preparation/address/README.md`',
    'Last-Reconciled-Commit: `5e7909f3e`',
):
    assert item in address_metadata, item
assert 'Current-Blocker:' not in address
assert 'AddressPlanAnalysisStageUnsupported' not in address
assert 'Step 6 remain blocked' not in address

address_core = ('## Purpose', '## Owns', '## Does Not Own', '## Inputs',
                '## Outputs', '## Adjacent-Stage Contract')
positions = [address.index(heading) for heading in address_core]
assert positions == sorted(positions)
assert positions[-1] < address.index('## Stable Identity and Exact Product Key')

assert len(rows(address, '### Exact C6 input matrix',
                'The shared [Provenance contract')) == 13
assert len(rows(address, '### Exact C6 output matrix',
                '## Adjacent-Stage Contract')) == 14
assert len(rows(address, '## Stable Identity and Exact Product Key',
                '## Exhaustive Address-Form Matrix')) == 11
assert len(rows(address, '## Exhaustive Address-Form Matrix',
                '## Ordered Behavior')) == 37

for item in (
    'admitted at exact B8',
    'CFG, dominance, publication/value-flow and memory-effects dependencies',
    'reject every B4, mixed-revision or implicitly refreshed handle',
    'global object', 'thread-local object', 'string or constant object',
    'semantic label address', 'block address', 'static local object',
    'dynamic stack object', 'constant-field GEP', 'variable-index GEP',
    'absolute/section-relative relocation admitted',
    'PC-relative/GOT/PLT relocation admitted', 'TLS relocation admitted',
    'generic pointer with Unknown origin and legal base rule',
    'name/text/host-pointer/numeric-address-only evidence',
    'Failure publishes no plan', 'old handles never retarget',
    'C7 inline-assembly target preparation', 'D4 target legalization',
    'E4 allocated publication', 'F1 is apply-only',
    'Implementation is absent', 'directory contains only this README',
):
    assert item in address_flat, item

for link in re.findall(r'\[[^]]+\]\(([^)]+)\)', address):
    assert (address_path.parent / link).resolve().exists(), link

for pattern in (
    'AnalysisId::Provenance|ProvenanceAnalysis|ProvenanceResult',
    'AddressPlan|AddressRequirementId|AddressPlanFingerprint',
):
    result = subprocess.run(
        ['rg', '-n', '--glob', '*.{cpp,hpp,h,cc,cxx}', pattern,
         'src/backend/bir'], text=True, capture_output=True)
    assert result.returncode == 1, result.stdout

plan = Path('plan.md').read_text()
plan_flat = ' '.join(plan.split())
for item in (
    'one narrow coordinated documentation-only repair is authorized',
    '`src/backend/bir/analysis/provenance/README.md`, then',
    '`src/backend/bir/preparation/address/README.md`',
):
    assert item in plan_flat, item

changed = subprocess.check_output(
    ['git', 'status', '--short', '--untracked-files=all'],
    text=True).splitlines()
changed_paths = {line[3:] for line in changed}
assert changed_paths == {
    'plan.md',
    'src/backend/bir/analysis/provenance/README.md',
    'src/backend/bir/preparation/address/README.md',
    'todo.md',
}, changed_paths
assert subprocess.run(['git', 'diff', '--check']).returncode == 0

todo_header = '\n'.join(Path('todo.md').read_text().splitlines()[:10])
assert 'Current Step ID: 7' in todo_header
assert 'Current Step Title: Converge C7 inline-assembly target vocabulary' in todo_header

print('PASS step=6 provenance=10/11 applicability=B4+B8 '
      'deps=Cfg+Dom+PVF+Effects-same-revision statuses=closed '
      'address=13/14 key=11 forms=37 c7+d4+e4+f1=exact '
      'implementation=absent scope=authorized-plan+provenance+address+todo '
      'next=7')
PY
```

- Result:

```text
PASS step=6 provenance=10/11 applicability=B4+B8 deps=Cfg+Dom+PVF+Effects-same-revision statuses=closed address=13/14 key=11 forms=37 c7+d4+e4+f1=exact implementation=absent scope=authorized-plan+provenance+address+todo next=7
```
