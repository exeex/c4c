# Current Packet

Status: Active
Source Idea Path: ideas/open/737_bir_phase_c_preparation_document_convergence.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Resolve the external C1 Markdown boundary and Canonical input

## Just Finished

- Audited plan Step 1 read-only. The accepted B8/C1 semantic handoff is
  unambiguous: C1 receives only the exact verifier-gated, move-only,
  target-independent, unallocated `CanonicalBir` plus an explicit target
  request. Raw aliases, reconstructed views/stamps, target facts already in
  BIR, stale analyses, prepared facts, ABI placement, homes, spills,
  allocation and MIR state are forbidden.
- Confirmed C1 must normalize and validate requested triple, architecture, OS,
  backend ABI, relocation model, floating-point ABI and enabled capabilities,
  then publish one immutable exact `TargetProfile` plus one complete
  `TargetFingerprint`/validated target-context key. Selection/validation
  failure publishes neither key nor `VerifiedPreparationInput` and never
  mutates Raw or Canonical storage.
- Confirmed authority separation: the external C1 owner selects, normalizes,
  validates and fingerprints the target request; the shared verifier alone
  binds that validated fingerprint to the exact Canonical
  `PipelineStageStamp` and returns `VerifiedPreparationInput`; C2 alone derives
  `VerifiedTargetLayout`. C1 owns no layout, preparation, constraint,
  allocation or analysis result.
- Found no explicit current Markdown authority for C1. The root C1 owner cell
  is unlinked prose, C2 links directly to `src/target_profile.hpp`, and the
  only other target-profile Markdown is historical research or downstream
  consumer material without current owner metadata. Step 1 therefore remains
  active pending a coordinated documentation boundary packet.
- The exact proposed placeholder/owner is
  `src/target_profile/README.md`, titled external target-profile selection and
  validation boundary. Required metadata is:
  `Contract-Status: under-review`,
  `Implementation-Status: partial-foundation`, `Kind: external-boundary`,
  `Phase-ID: C1`, exact Canonical-plus-request `Upstream`, validated
  profile/fingerprint-plus-verifier/C2 `Downstream`, exact `Owner-Path`, and
  `Last-Reconciled-Commit`.
- The proposed owner must be core-first (`Purpose`, `Owns`, `Does Not Own`,
  `Inputs`, `Outputs`, `Adjacent-Stage Contract`) and contain exhaustive input
  and output matrices. Inputs must separately cover the exact B8 Canonical
  capability/stamp and requested triple, arch, OS, ABI, relocation, float ABI,
  capabilities and selection/schema versions. Outputs must cover one immutable
  validated `TargetProfile`, one exact all-axis `TargetFingerprint`/context
  key, structured failure, atomic no-output failure and no BIR mutation.
- Required ownership clauses: C1 alone owns request normalization, cross-field
  coherence/support validation, defaulting rules and fingerprint formation;
  it cannot choose a target for the user, accept compatible-looking/partial
  requests, parse rendered `data_layout`, derive C2 layout, run C3-C9, create
  `VerifiedPreparationInput`, mutate BIR or use analyses as acceptance
  authority.
- Current implementation is only partial foundation. Build-included
  `src/target_profile.hpp/.cpp` provides the `TargetProfile` value, triple
  parser/defaults/name helpers and fields for triple, arch, OS, backend ABI,
  relocation and two coarse float-register capability booleans. It has no
  `TargetFingerprint`, validated target-context key, complete capability set,
  explicit all-axis request/coherence validator, Canonical input, or C1/
  `VerifiedPreparationInput` publication gate. Existing frontend/LIR/legacy
  consumers are implementation evidence, not the post-Canonical C1 owner.
- No shared analysis belongs at C1. Publication/value-flow, call graph,
  provenance, memory effects and other analyses remain at their later declared
  earliest consumers.

## Suggested Next

- Authorize one coordinated Step-1 documentation packet that creates
  `src/target_profile/README.md` and repairs all two-sided links/authority seams
  together. Keep Step 1 active until that packet is complete and proven; do
  not advance to C2 from this audit.

## Watchouts

- Exact coordinated seam set:
  `src/target_profile/README.md` (new sole external C1 owner),
  `src/backend/bir/README.md` (link the normative C1 owner cell and preserve
  exact B8->C1->C2 order),
  `src/backend/bir/pipeline/README.md` (accepted closed-Child-B downstream edge
  must link C1 without weakening the Canonical contract),
  `src/backend/bir/verify/README.md` (link target-selection authority while
  retaining sole `VerifiedPreparationInput` binding/gate authority),
  `src/backend/bir/target_layout/README.md` (replace header-as-authority with
  the C1 owner and consume its exact validated key), and
  `src/backend/bir/preparation/README.md` (link the C1 producer in the boundary
  and product table). Because pipeline/verifier are accepted shared seams,
  authorize the set before editing any member.
- `docs/target_abi_contract_research/*` is useful historical evidence but must
  not become the normative C1 owner or receive lifecycle edits.
- Preserve the accepted semantic-size/alignment/address-space distinction:
  source-semantic facts may remain in Canonical BIR, but selected target
  profile/layout/ABI facts cannot.
- Do not claim current `target_profile_from_triple` is validation or
  fingerprint publication. It derives a subset from a triple, leaves
  relocation mutable/defaulted, exposes directly constructible public fields,
  and is not bound to Canonical revision identity.
- Do not edit code/tests/build/logs, invent implementation, activate Child D or
  move shared analyses earlier.

## Proof

- Documentation/implementation-truth audit only. The delegated scope forbids
  code, test, build and log edits; no `test_after.log` was created or modified.
- Exact scope/diff, owner inventory, link evidence, C1 input/output/exclusion,
  accepted Canonical handoff and implementation/build-truth proof:

```bash
python3 - <<'PY'
from pathlib import Path
import re
import subprocess

root_path = Path('src/backend/bir/README.md')
pipeline_path = Path('src/backend/bir/pipeline/README.md')
verify_path = Path('src/backend/bir/verify/README.md')
layout_path = Path('src/backend/bir/target_layout/README.md')
preparation_path = Path('src/backend/bir/preparation/README.md')
header_path = Path('src/target_profile.hpp')
source_path = Path('src/target_profile.cpp')
cmake_path = Path('CMakeLists.txt')
proposed_path = Path('src/target_profile/README.md')
idea_path = Path('ideas/open/737_bir_phase_c_preparation_document_convergence.md')

root = root_path.read_text()
pipeline = pipeline_path.read_text()
verify = verify_path.read_text()
layout = layout_path.read_text()
preparation = preparation_path.read_text()
header = header_path.read_text()
source = source_path.read_text()
cmake = cmake_path.read_text()
idea = idea_path.read_text()

# One exact accepted Canonical input, explicit request, validated-key output,
# and no BIR mutation are already coherent across the accepted boundary.
c1 = next(line for line in root.splitlines() if line.startswith('| `C1` |'))
assert '`CanonicalBir` plus requested triple/arch/OS/ABI/relocation/float-ABI capabilities' in c1
assert 'one exact validated target-context key; no BIR mutation' in c1
assert 'external target-profile authority' in c1
assert '](' not in c1.split('|')[-2], c1
for item in (
    'Downstream: one verifier-gated immutable target-independent unallocated `CanonicalBir`',
    'immutable `CanonicalBir` | C1 and read-only observers',
    '`CanonicalBir`. C1 receives only the published immutable result.',
    'one immutable\ntarget-independent unallocated `CanonicalBir` carrying the verified stamp',
    'C1 selects target context independently after publication',
):
    assert item in pipeline, item
for item in (
    'C1 target selection + Canonical --verify_preparation_input--> VerifiedPreparationInput',
    'one already-published `CanonicalBir` plus one validated `TargetProfile`',
    'same owning revision frozen by P07',
    'stale analysis result',
    'Canonical facts remain target-independent',
    'B8 rejects every stage-forbidden\nprepared, calling-placement, allocation, frame',
    'Raw and Canonical BIR carry no semantic `target_profile`',
    'Neither decision is imported into Raw or Canonical\nstorage',
    '`verify_preparation_input` is the only target-bound C1 input gate',
    'It is non-mutating, publishes no\n   BIR revision or prepared fact',
):
    assert item in verify, item
assert 'VerifiedPreparationInput` | C1 `verify_preparation_input` gate' in preparation
assert 'It does not create a new BIR revision' in preparation

# C1's required target axes and exact-key output are explicit in adjacent C2.
for item in ('architecture, triple/OS, backend ABI, relocation model',
             'floating-point ABI', 'enabled capabilities',
             'layout-schema version', 'concrete-mapping-table\nversion'):
    assert item in layout, item
assert 'one `TargetFingerprint`' in layout
assert 'Field-by-field equality' in layout
assert 'Input is the exact validated `TargetProfile`' in layout
assert 'Failure publishes no layout or partial table. Inputs remain unchanged' in layout

# No current normative Markdown owner exists. Historical research is evidence,
# while the root is unlinked and C2 links directly to a header.
assert not proposed_path.exists()
markdown_hits = subprocess.check_output(
    ['rg', '-l', '-i', 'TargetProfile|target profile|target-profile',
     '--glob', '*.md', '.'], text=True).splitlines()
assert markdown_hits
owner_claims = []
for hit in markdown_hits:
    if hit == './todo.md':
        continue
    text = Path(hit).read_text()
    if ('Owner-Path: `src/target_profile/' in text or
            'Phase-ID: C1' in '\n'.join(text.splitlines()[:15])):
        owner_claims.append(hit)
assert owner_claims == [], owner_claims
assert '[`TargetProfile`](../../../target_profile.hpp)' in layout
research = Path('docs/target_abi_contract_research/index.md').read_text()
assert 'research' in research.lower()
assert 'src/target_profile.hpp' in Path(
    'docs/target_abi_contract_research/01_how_target_information_enters_the_pipeline.md'
).read_text()

# Current code/build truth is a build-included value/parser foundation only.
for item in ('enum class TargetArch', 'enum class TargetOs',
             'enum class BackendAbiKind', 'enum class TargetRelocationModel',
             'struct TargetProfile', 'std::string triple;',
             'TargetArch arch', 'TargetOs os', 'BackendAbiKind backend_abi',
             'TargetRelocationModel relocation_model',
             'bool has_float_arg_registers',
             'bool has_float_return_registers',
             'target_profile_from_triple(std::string_view target_triple)'):
    assert item in header, item
for item in ('TargetProfile target_profile_from_triple',
             'profile.triple = std::string(target_triple)',
             'profile.arch = TargetArch::', 'profile.os = os_from_triple',
             'profile.backend_abi = backend_abi_from_triple',
             'profile.has_float_arg_registers =',
             'profile.has_float_return_registers ='):
    assert item in source, item
assert 'profile.relocation_model =' not in source
assert 'add_library(c4c_target_profile STATIC' in cmake
assert '"${PROJECT_SOURCE_DIR}/src/target_profile.cpp"' in cmake
assert 'target_link_libraries(c4c_backend PUBLIC c4c_frontend c4c_codegen c4c_target_profile)' in Path(
    'src/backend/CMakeLists.txt').read_text()
code = '\n'.join(path.read_text() for path in Path('src/backend/bir').rglob('*')
                 if path.suffix in {'.hpp', '.cpp'})
assert 'class CanonicalBir' in code
for absent in ('struct TargetFingerprint', 'class VerifiedPreparationInput',
               'verify_preparation_input('):
    assert absent not in code, absent
module_data = Path('src/backend/bir/core/ir.hpp').read_text().split(
    'struct ModuleData {', 1)[1].split('};', 1)[0]
for forbidden in ('TargetProfile', 'target_profile', 'BackendAbiKind',
                  'home', 'spill', 'allocation', 'MIR'):
    assert forbidden not in module_data, forbidden

# Exact coordinated seams exist and local links currently resolve.
seams = (root_path, pipeline_path, verify_path, layout_path, preparation_path)
for path in seams:
    text = path.read_text()
    for link in re.findall(r'\[[^]]+\]\(([^)]+)\)', text):
        if '://' in link or link.startswith('#'):
            continue
        assert (path.parent / link.split('#', 1)[0]).resolve().exists(), (path, link)
assert 'external target-profile authority' in root
assert 'C1 receives only the published immutable result' in pipeline
assert '`verify_preparation_input` is the only target-bound C1 input gate' in verify
assert '[`TargetProfile`](../../../target_profile.hpp)' in layout
assert 'C1 `verify_preparation_input` gate' in preparation

# Shared analyses remain downstream, never C1 selection/acceptance authority.
for analysis in ('PublicationValueFlow', 'CallGraph', 'Provenance', 'MemoryEffects'):
    assert analysis not in c1, analysis
analysis_index = Path('src/backend/bir/analysis/README.md').read_text()
assert 'CanonicalSemantic' in analysis_index and 'TargetBoundAllocation' in analysis_index

# Step remains active and only canonical todo state changed.
status = subprocess.check_output(['git', 'status', '--short'], text=True)
assert status == ' M todo.md\n', status
changed = subprocess.check_output(['git', 'diff', '--name-only'], text=True).splitlines()
assert changed == ['todo.md'], changed
assert 'Current Step ID: 1' in Path('todo.md').read_text()
assert 'Status: Active' in Path('todo.md').read_text()
assert 'if so, record the exact path/owner/two-sided seam and stop' in Path('plan.md').read_text()
assert ('A missing real Markdown owner may be created only as a documentation '
        'placeholder') in ' '.join(idea.split())

print('PASS step=1 status=retained owner=current-missing '
      'proposed=src/target_profile/README.md seams=5+new '
      'canonical=exact-verifier-gated-target-independent-unallocated '
      'request=triple+arch+os+abi+relocation+float-abi+capabilities '
      'output=validated-profile+exact-target-fingerprint mutation=none '
      'authority=external-selection/verifier-binding/c2-layout-separated '
      'implementation=partial-foundation build=c4c_target_profile '
      'c1-gate=absent analyses=later-only scope=todo-only links=green')
PY
```

- Result:

```text
PASS step=1 status=retained owner=current-missing proposed=src/target_profile/README.md seams=5+new canonical=exact-verifier-gated-target-independent-unallocated request=triple+arch+os+abi+relocation+float-abi+capabilities output=validated-profile+exact-target-fingerprint mutation=none authority=external-selection/verifier-binding/c2-layout-separated implementation=partial-foundation build=c4c_target_profile c1-gate=absent analyses=later-only scope=todo-only links=green
```
