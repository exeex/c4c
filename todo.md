# Current Packet

Status: Active
Source Idea Path: ideas/open/737_bir_phase_c_preparation_document_convergence.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Converge C2 target-layout derivation

## Just Finished

- Completed plan Step 1 under coordinated authorization `bd0a13414`.
  Created `src/target_profile/README.md` as the sole external C1 request
  selection, normalization, validation, defaulting/coherence and target-
  fingerprint owner, then repaired the exact two-sided C1 links in all five
  authorized shared documents.
- Added uniform metadata with `under-review`, `partial-foundation`,
  `external-boundary`, `C1`, exact Canonical-plus-request upstream, exact
  validated-profile/fingerprint downstream, owner path and authorization
  reconciliation point. Core-first ownership sections are substantive and all
  checklist items are resolved.
- Added an 11-row exhaustive input matrix covering the exact verifier-gated B8
  `CanonicalBir` capability/stamp plus explicit requested triple,
  architecture, OS, backend ABI, relocation, floating-point ABI, capabilities,
  selection version and request/profile/fingerprint schema versions.
- Added a four-row output matrix for exactly one immutable validated
  `TargetProfile`, one complete all-axis `TargetFingerprint`/context key,
  diagnostic replay identity and structured failure. Failure is atomic and
  publishes no profile, key, binding capability, cache entry or BIR revision;
  the exact Canonical owner and request remain unchanged.
- Preserved authority separation. External C1 alone selects/normalizes/
  validates/defaults the explicit request and fingerprints it, but never
  chooses a target for the user. The verifier alone inspects and binds the
  validated target fingerprint to the exact unchanged Canonical stamp and may
  produce `VerifiedPreparationInput`. C2 alone derives `VerifiedTargetLayout`.
- Preserved accepted Child-B semantics: only the completely green B8 owner
  reaches C1, Canonical remains target-independent and unallocated, and no
  Raw/view/report alias, target fact, ABI placement, home, spill, allocation,
  MIR state or analysis can become C1 acceptance authority.
- Corrected the root/index and both sides of every shared seam: normative root
  C1 owner, pipeline B8 downstream edge, verifier binding authority, target-
  layout C1 input authority and preparation product producer now link the sole
  C1 owner without reordering B8 -> C1 -> C2.
- Kept implementation truth exact. Build-included `target_profile.hpp/.cpp`
  is only partial foundation: mutable value/enums, triple parser/defaults/name
  helpers and two coarse float capability booleans. Closed `TargetRequest`,
  complete capability registry, immutable validated-profile capability,
  `TargetFingerprint`, structured C1 failure, Canonical-bound entry point and
  `VerifiedPreparationInput` gate integration remain absent.
- Re-audited the complete authorized slice: exactly six documentation paths
  plus canonical `todo.md`; unique owner, metadata, core order, 11/4 matrices,
  links, authority, atomic failure, implementation truth and Step-2 handoff are
  green. No code, tests, build, logs, plan, source idea or other file changed.

## Suggested Next

- Execute plan Step 2 against `src/backend/bir/target_layout/README.md`: converge
  C2 metadata/core-first structure, exhaustive input/output and finite-layout
  matrices, exact profile/schema/options keys, failure atomicity and C3
  acceptance while preserving C1's sole target-profile authority.

## Watchouts

- C2 must consume the exact C1 validated profile/fingerprint and verifier-bound
  `VerifiedPreparationInput`; it cannot reconstruct target identity from
  architecture/triple equality or parse Raw/Canonical `data_layout` text.
- `TargetFingerprint` remains target-context identity. The verifier alone binds
  it to the exact Canonical `PipelineStageStamp`; C2's published layout must
  carry both without stealing either authority.
- C2 owns finite categories/classes/groups/slots/aliases/reserved units,
  capacity, ABI eligibility and the private concrete-mapping domain. It owns no
  per-value ABI classification, preparation, constraints, assignment, spill,
  frame or machine operation.
- Shared publication/value-flow and other analyses begin at their later
  declared earliest consumers, not in C1 or C2 target selection/layout
  authority unless Step 2's source explicitly requires a real consumer.
- Documentation convergence does not authorize C1/C2 implementation. Do not
  edit code/tests/build/logs or weaken the partial-foundation/absent truth.

## Proof

- Documentation-only packet. The delegated scope forbids code, test, build and
  log edits; no `test_after.log` was created or modified.
- Exact authorized-scope, unique-owner, metadata/core, matrix, link,
  B8/C1/C2-order, authority, failure, implementation-truth and Step-2 handoff
  proof:

```bash
python3 - <<'PY'
from pathlib import Path
import re
import subprocess

owner = Path('src/target_profile/README.md')
root = Path('src/backend/bir/README.md')
pipeline = Path('src/backend/bir/pipeline/README.md')
verifier = Path('src/backend/bir/verify/README.md')
layout = Path('src/backend/bir/target_layout/README.md')
preparation = Path('src/backend/bir/preparation/README.md')
docs = (owner, root, pipeline, verifier, layout, preparation)
text = {path: path.read_text() for path in docs}
flat = {path: ' '.join(text[path].split()) for path in docs}

# Unique owner metadata and core-first structure.
metadata = '\n'.join(text[owner].splitlines()[:11])
for item in (
    'Contract-Status: under-review',
    'Implementation-Status: partial-foundation',
    'Kind: external-boundary',
    'Phase-ID: C1',
    'Upstream: one exact B8 verifier-gated target-independent unallocated `CanonicalBir` plus one explicit `TargetRequest`',
    'Downstream: one immutable validated `TargetProfile` plus complete `TargetFingerprint` consumed by the C1 verifier binding and C2',
    'Owner-Path: `src/target_profile/README.md`',
    'Last-Reconciled-Commit: `bd0a13414`',
):
    assert item in metadata, item
core = ('## Purpose', '## Owns', '## Does Not Own', '## Inputs',
        '## Outputs', '## Adjacent-Stage Contract')
positions = [text[owner].index(heading) for heading in core]
assert positions == sorted(positions), positions
assert '- [ ]' not in text[owner]

claims = []
for candidate in Path('.').rglob('*.md'):
    if not candidate.is_file() or candidate == Path('todo.md'):
        continue
    candidate_text = candidate.read_text(errors='replace')
    if ('Phase-ID: C1' in '\n'.join(candidate_text.splitlines()[:15]) or
            'Owner-Path: `src/target_profile/README.md`' in candidate_text):
        claims.append(str(candidate))
assert claims == ['src/target_profile/README.md'], claims

# Exact exhaustive matrices.
def matrix_rows(start, end):
    section = text[owner].split(start, 1)[1].split(end, 1)[0]
    return [line for line in section.splitlines() if line.startswith('|')][2:]

inputs = matrix_rows('### Exact C1 input matrix', '## Outputs')
outputs = matrix_rows('### Exact C1 output matrix',
                      '## Adjacent-Stage Contract')
assert (len(inputs), len(outputs)) == (11, 4)
for axis in (
    'Canonical capability', 'Canonical stamp', 'requested triple',
    'requested architecture', 'requested OS', 'requested backend ABI',
    'requested relocation model', 'requested floating-point ABI',
    'requested capabilities', 'selection version', 'schema versions',
):
    assert any(axis in row for row in inputs), axis
for product in (
    'validated immutable `TargetProfile`', 'complete `TargetFingerprint`',
    'normalized request record', 'structured `TargetSelectionFailure`',
):
    assert any(product in row for row in outputs), product
for item in (
    'never chooses a target for the user',
    'does not inspect, validate, copy or mutate either one',
    'parsing rendered `data_layout`',
    'publishes no profile, fingerprint',
    'No partial profile, key, cache entry, `VerifiedPreparationInput` or BIR revision is published',
    'Analyses remain later exact-revision dependencies',
):
    assert item in flat[owner], item

# Every local link resolves and every authorized shared seam links the owner.
for path in docs:
    for link in re.findall(r'\[[^]]+\]\(([^)]+)\)', text[path]):
        if link.startswith('#') or '://' in link:
            continue
        destination = (path.parent / link.split('#', 1)[0]).resolve()
        assert destination.exists(), (path, link, destination)
assert '../../target_profile/README.md' in text[root]
for path in (pipeline, verifier, layout, preparation):
    assert '../../../target_profile/README.md' in text[path], path

# Exact B8 -> C1 -> C2 order and accepted Canonical semantics remain intact.
stage_ids = [
    re.match(r'^\| `([A-F][0-9])`', line).group(1)
    for line in text[root].splitlines()
    if re.match(r'^\| `[A-F][0-9]`', line)
]
assert stage_ids.index('B8') + 1 == stage_ids.index('C1')
assert stage_ids.index('C1') + 1 == stage_ids.index('C2')
for item in (
    'receives only the published immutable result',
    'Only the completely green',
    'target-independent unallocated `CanonicalBir` carrying the verified stamp',
):
    assert item in flat[pipeline], item

# External selection, verifier binding and C2 layout authorities are distinct.
for item in (
    'external C1 target-profile boundary',
    'alone selects, normalizes and validates the explicit target request',
    'it alone binds that validated fingerprint',
    'only target-bound C1 input gate',
):
    assert item in flat[verifier], item
for item in (
    "sole owner of the target's abstract register vocabulary",
    'Input is the exact validated `TargetProfile`',
    'one `TargetFingerprint`',
):
    assert item in flat[layout], item
assert 'external C1 target-profile boundary' in flat[preparation]
for item in (
    '| explicit request normalization, validation, registered support and target fingerprint | external C1 boundary |',
    '| full Canonical rules and exact Canonical/target binding into `VerifiedPreparationInput` | BIR verifier |',
    '| finite target register/layout vocabulary and `VerifiedTargetLayout` | C2 target layout |',
):
    assert item in text[owner], item

# Current implementation/build truth remains partial foundation with no gate.
header = Path('src/target_profile.hpp').read_text()
source = Path('src/target_profile.cpp').read_text()
cmake = Path('CMakeLists.txt').read_text()
for item in (
    'struct TargetProfile', 'TargetRelocationModel relocation_model',
    'bool has_float_arg_registers', 'bool has_float_return_registers',
    'target_profile_from_triple(std::string_view target_triple)',
):
    assert item in header, item
assert 'TargetProfile target_profile_from_triple' in source
assert 'add_library(c4c_target_profile STATIC' in cmake
assert '"${PROJECT_SOURCE_DIR}/src/target_profile.cpp"' in cmake
code = '\n'.join(
    path.read_text()
    for path in Path('src/backend/bir').rglob('*')
    if path.is_file() and path.suffix in {'.hpp', '.cpp'}
)
for absent in ('struct TargetFingerprint', 'class VerifiedPreparationInput',
               'verify_preparation_input('):
    assert absent not in code, absent
for item in (
    'Implementation is partial foundation only',
    'Absent are the closed `TargetRequest`',
    'no checked-in implementation can produce C1 success',
):
    assert item in flat[owner], item

# Exact diff scope: six authorized docs plus canonical todo only.
expected = {
    'src/target_profile/README.md',
    'src/backend/bir/README.md',
    'src/backend/bir/pipeline/README.md',
    'src/backend/bir/verify/README.md',
    'src/backend/bir/target_layout/README.md',
    'src/backend/bir/preparation/README.md',
    'todo.md',
}
changed = set(subprocess.check_output(
    ['git', 'status', '--short', '--untracked-files=all'],
    text=True).splitlines())
changed_paths = {line[3:] for line in changed}
assert changed_paths == expected, (changed_paths, expected)
for path in changed_paths:
    assert not path.endswith(('.cpp', '.hpp', '.h', '.cc', '.c', '.log'))
    assert not path.startswith(('tests/', 'build/', 'ideas/'))
assert 'Current Step ID: 2' in Path('todo.md').read_text()
assert 'Current Step Title: Converge C2 target-layout derivation' in Path(
    'todo.md').read_text()

print('PASS step=1 owner=unique-c1 metadata=8 core=6 matrices=11/4 '
      'order=B8-C1-C2 canonical=exact-verifier-gated-target-independent-unallocated '
      'request=triple+arch+os+abi+relocation+float-abi+capabilities+versions '
      'output=one-profile+one-fingerprint failure=atomic-no-bir-mutation '
      'authority=external-selection/verifier-binding/c2-layout-separated '
      'implementation=partial-foundation gate=absent links=6-green '
      'scope=6-docs+todo next=2')
PY
```

- Result:

```text
PASS step=1 owner=unique-c1 metadata=8 core=6 matrices=11/4 order=B8-C1-C2 canonical=exact-verifier-gated-target-independent-unallocated request=triple+arch+os+abi+relocation+float-abi+capabilities+versions output=one-profile+one-fingerprint failure=atomic-no-bir-mutation authority=external-selection/verifier-binding/c2-layout-separated implementation=partial-foundation gate=absent links=6-green scope=6-docs+todo next=2
```
