# Current Packet

Status: Active
Source Idea Path: ideas/open/735_bir_phase_a_import_raw_document_convergence.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Prove phase-B acceptance and the deferred idea-734 boundary

## Just Finished

- Completed plan Step 4 by repairing exactly the authorized five-document A2
  shared boundary and freshly re-auditing every named shared row against the
  accepted phase-A comparison owners. No sixth document was needed.
- `verify/README.md` now excludes semantic target profile/triple/rendered
  layout/language-ABI/pointer-layout and all other C1/C2 context from Raw and
  Canonical storage. Validation-only origin/parity remains non-semantic, C1
  independently selects the exact `TargetProfile`, and C2 derives layout.
- The verifier's import and coverage ledgers now defer to the accepted complete
  immutable 38/6/18 current-LIR matrices. Existing facts keep their named
  container/wiring/validation dispositions; desired absent forms are outside
  intake and do not authorize LIR/schema work.
- Compatibility and legacy coverage now defer missing/current facts to the
  owning phase-A matrix and remain observation only. B1 reports inherited A2
  publication/contract failure and is never a producer-repair route. The root
  index now records idea 731 as closed bounded historical proof only.
- Preserved the sole exact-revision
  `verify_and_publish_raw(ModuleDraft&&)` gate, full verification, failure
  atomicity, stable identity/CFG authority, exact-revision analysis and stale
  rejection, read-only diagnostics/quarantine, review gates, immutable B1
  input and root phase order.

### Fresh shared-boundary audit matrix

| Audited owner and clause | Result | Fresh post-edit finding |
|---|---|---|
| `verify/README.md`, **Profiles and stage boundary** plus **Transactional construction and publication** | Compatible | The private move-only exact-draft path remains the sole full Raw publication gate. Candidate checks are diagnostic-only; failure publishes no function subset, partial capability, cache fact or alternate stage token. |
| `verify/README.md`, **Validation phase order** plus **Module, types, symbols, and globals / Type universe** | Compatible | Phase 2 validates only target-independent semantic state. Raw/Canonical exclude semantic target/profile/layout/ABI context and non-semantic origin/parity cannot create facts; C1/C2 retain independent profile/layout authority. |
| `verify/README.md`, **LIR import gate and current-intake dispositions** plus **Backend coverage status ledger** | Compatible | The complete immutable 38/6/18 matrices control current receipt. Current facts retain exact phase-A dispositions; absent desired forms are outside intake, not LIR/schema work, and text/legacy placeholders remain forbidden. |
| `analysis/README.md`, **Scope and ownership**, **Exact revision keys**, **Computation**, **Transaction**, and **Invalidation** | Compatible | Results remain immutable exact-revision consumers keyed by stable IDs; stale/foreign keys reject, mutation invalidates, failure publishes no result, and analyses cannot repair core facts or mint stage tokens. |
| `diagnostics/README.md`, **Borrow and revision**, **Structured diagnostics**, **Deterministic rendering**, and **Failure and ownership** | Compatible | Observations remain read-only and exact-revision bound; presentation cannot alter success/severity/order, construct or publish stages, repair missing facts, or feed rendered text back as authority. |
| `compatibility/README.md`, **Quarantine boundary**, **Revision and lifetime rules**, and **Deletion discipline** | Compatible | Missing facts defer to their owning accepted matrix. Quarantine remains post-producer observation with an empty production manifest and cannot create, repair, override or select semantic facts. |
| `LEGACY_COVERAGE.md`, typed import/error row and ledger rules | Compatible | The row still rejects text recovery and partial publication, now defers every current fact to its exact phase-A receiving disposition, and forbids legacy evidence from reopening LIR or creating/repairing facts. |
| `REVIEW_TEMPLATE.md`, **Complete boundary walk**, **Ownership and observation review**, **Legacy disposition review**, and **Same-family and anti-overfit proof** | Compatible | Review still requires exact owner/revision/verifier/failure/consumer evidence, one full atomic Raw gate, read-only consumers, one legacy disposition and neighboring proof without expectation weakening. |
| Root `README.md`, A1/A2/B1 stage table, publication/invalidation/cross-cutting rules and architecture checkpoint | Compatible | Target-independent A1/A2/B1 order and single-gate authority are unchanged. Idea 731 is accurately closed as bounded non-goto inline-asm transport history without implying phase, semantic, implementation or activation change. |
| `passes/legalize/README.md`, **Exact input checkpoint**, **Closed authority**, **Transaction**, and **Unsupported and failure behavior** | Compatible | B1 accepts only one immutable exact-revision fully verified `RawBir`, has no target/repair authority, reports malformed upstream state as inherited A2 failure, and publishes nothing on failure. |

## Suggested Next

- Execute plan Step 5 as a phase-A handoff proof: verify the exhaustive A1
  private-draft through A2 `RawBir` output matrices against phase B's exact
  immutable input, and keep idea 734 only as the inactive implementation
  consumer. Request plan-owner lifecycle judgment after the docs-only proof;
  do not activate phase B or idea 734.

## Watchouts

- The verifier's full target-independent schema can describe desired Raw forms
  that are outside current intake; that does not make them current-LIR gaps or
  implementation coverage and never authorizes LIR/schema edits.
- Preserve `target_profile` and rendered `data_layout` as validation/origin/
  parity-only non-destinations through the Step-5 adjacency proof.
- Idea 731's closed status is bounded history only. Idea 734 remains deferred,
  and the open umbrella remains the lifecycle owner for later children.

## Proof

- Docs-only packet: the delegated proof explicitly requires no canonical logs;
  neither `test_before.log` nor `test_after.log` changed.
- Fresh link, former-conflict removal, preserved-authority, repaired-boundary,
  exact diff-scope and whitespace proof:

```bash
python3 - <<'PY'
from pathlib import Path
import re
import subprocess

paths = [Path(path) for path in [
    'src/backend/bir/lir_to_bir/README.md',
    'src/backend/bir/lir_to_bir/memory/README.md',
    'src/backend/bir/core/README.md',
    'src/backend/bir/verify/README.md',
    'src/backend/bir/analysis/README.md',
    'src/backend/bir/diagnostics/README.md',
    'src/backend/bir/compatibility/README.md',
    'src/backend/bir/LEGACY_COVERAGE.md',
    'src/backend/bir/REVIEW_TEMPLATE.md',
    'src/backend/bir/README.md',
    'src/backend/bir/passes/legalize/README.md',
]]
assert all(path.is_file() for path in paths)
docs = {path.as_posix(): path.read_text() for path in paths}

link_count = 0
for path in paths:
    text = path.read_text()
    text = re.sub(r'```.*?```', '', text, flags=re.S)
    text = re.sub(r'`[^`]*`', '', text)
    for target in re.findall(r'\[[^]]*\]\(([^)#]+)(?:#[^)]*)?\)', text):
        if '://' in target:
            continue
        resolved = (path.parent / target).resolve()
        assert resolved.exists(), (path, target, resolved)
        link_count += 1

verify = docs['src/backend/bir/verify/README.md']
analysis = docs['src/backend/bir/analysis/README.md']
diagnostics = docs['src/backend/bir/diagnostics/README.md']
compatibility = docs['src/backend/bir/compatibility/README.md']
legacy = docs['src/backend/bir/LEGACY_COVERAGE.md']
review = docs['src/backend/bir/REVIEW_TEMPLATE.md']
root = docs['src/backend/bir/README.md']
legalize = docs['src/backend/bir/passes/legalize/README.md']

former_conflicts = [
    (verify, 'immutable\n   lowering-environment metadata'),
    (verify, 'target triple,\ndata-layout version, pointer widths/address spaces, and language ABI mode'),
    (verify, '## LIR import gate and explicit source gaps'),
    (verify, 'missing typed initializer references are a source gap'),
    (verify, 'explicit producer-schema gap'),
    (compatibility, 'A missing typed fact is a producer gap'),
    (legacy, 'missing structured source carriers remain explicit source failures'),
    (legalize, 'other producer gaps are Raw\npublication failures'),
    (root, 'Idea\n731 remains open'),
]
for text, conflict in former_conflicts:
    assert conflict not in text, conflict

preserved_rules = [
    (verify, 'Only\n`verify_and_publish_raw(ModuleDraft&&)`'),
    (verify, '`verify_candidate` is diagnostic-only'),
    (verify, 'publishes no function subset'),
    (analysis, 'An analysis result is an immutable, disposable view of one exact published BIR\nrevision'),
    (analysis, 'A mismatch returns structured\n`StaleAnalysis`'),
    (diagnostics, 'read-only, non-authoritative observations'),
    (diagnostics, 'never semantic identity'),
    (compatibility, 'cannot be consulted by import acceptance'),
    (compatibility, 'change verifier severity'),
    (legacy, 'reject text recovery and partial publication'),
    (review, 'does full Raw verification atomically publish the exact frozen draft'),
    (root, '| `A2` | Draft/Raw verification and publication | `ModuleDraft` | verified, target-independent, unallocated `RawBir` |'),
    (legalize, 'The only input is an immutable, published `RawBir` view'),
    (legalize, 'Failure publishes no revision, property, partial rewrite, analysis result, or\nstage token'),
]
for text, rule in preserved_rules:
    assert rule in text, rule

repaired_rules = [
    (verify, 'Raw and Canonical BIR carry no semantic `target_profile`, target triple,\nrendered `data_layout`, language-ABI mode, pointer-width/address-space layout\nselection, or other C1/C2 target context'),
    (verify, 'C1 independently selects\nand validates one exact `TargetProfile`; C2 derives the target-layout facts'),
    (verify, 'complete immutable current-LIR intake'),
    (verify, 'does not authorize LIR or producer-schema work'),
    (compatibility, 'follows the exact disposition in its owning accepted\nmatrix'),
    (compatibility, 'quarantine evidence cannot reopen LIR or prescribe a\nproducer/schema change'),
    (legacy, 'every current-LIR fact follows its exact accepted phase-A receiving disposition'),
    (legacy, 'legacy evidence cannot reopen LIR or create/repair semantic facts'),
    (legalize, 'inherited A2 publication or\ncontract failures'),
    (legalize, 'B1 is never a producer-repair route'),
    (root, 'Idea\n731 is closed as bounded historical proof of structured non-goto inline-asm\ntransport only'),
]
for text, rule in repaired_rules:
    assert rule in text, rule

changed = {
    line for line in subprocess.check_output(
        ['git', 'diff', '--name-only'], text=True).splitlines() if line
}
expected = {
    'src/backend/bir/verify/README.md',
    'src/backend/bir/compatibility/README.md',
    'src/backend/bir/LEGACY_COVERAGE.md',
    'src/backend/bir/passes/legalize/README.md',
    'src/backend/bir/README.md',
    'todo.md',
}
assert changed == expected, (changed, expected)
print(f'PASS docs={len(paths)} links={link_count} former_conflicts=0 '
      f'preserved_rules={len(preserved_rules)} repaired_rules={len(repaired_rules)} '
      'scope=five-docs+todo next_step=5')
PY
git diff --check
```

- Result:

```text
PASS docs=11 links=85 former_conflicts=0 preserved_rules=14 repaired_rules=11 scope=five-docs+todo next_step=5
git diff --check: PASS
```
