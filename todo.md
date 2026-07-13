# Current Packet

Status: Active
Source Idea Path: ideas/open/735_bir_phase_a_import_raw_document_convergence.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Audit the shared A2 verifier and cross-cutting boundaries

## Just Finished

- Executed plan Step 4 as a read-only audit of the committed phase-A contracts
  (`10d70b872`, `060a32c78`, `e759322a`) against every named shared/A2,
  cross-cutting, root and B1 owner. No `src/**` file was edited.
- Confirmed compatible authority for the private exact-revision
  `ModuleDraft -> verify_and_publish_raw(ModuleDraft&&) -> RawBir` boundary,
  diagnostic-only verifier non-authority, failure atomicity, stable IDs,
  deterministic order, exact def-use, terminator-only CFG successors,
  analysis revision/invalidation/stale rejection, read-only diagnostics,
  observational compatibility quarantine, legacy one-owner dispositions,
  review gates and B1's immutable exact-`RawBir` input.
- Found shared text that still contradicts accepted Child-A source/receiving
  ownership. Step 4 therefore remains active. The conflicts require an
  explicitly coordinated documentation packet; this audit did not edit them.

### Shared-boundary audit matrix

| Audited owner and clause | Compatibility result | Exact mismatch / disposition |
|---|---|---|
| `src/backend/bir/verify/README.md`, **Profiles and stage boundary** and **Transactional construction and publication** | Compatible | Names the sole move-only exact-draft full Raw gate, forbids builder/diagnostic bypass, records foundation adapters as partial, and publishes no subset/capability on failure. |
| `src/backend/bir/verify/README.md`, **Validation phase order**, **Module, types, symbols, and globals / Type universe**, **LIR import gate and explicit source gaps**, and final coverage table | **Blocking mismatch** | Validation phase 2 and the type-universe clause require semantic Raw “lowering-environment” state containing target triple, data-layout version, pointer widths/address spaces and language ABI mode. Child A assigns `target_profile` and rendered `data_layout` to validation/origin/parity-only non-destinations and reserves C1/C2 for target selection/layout. The import/coverage ledgers also call current initializer/call/atomic/inline-asm absences LIR “source” or “producer-schema” gaps and require future carriers, contradicting the complete/immutable current-LIR intake and receiving-side dispositions. |
| `src/backend/bir/analysis/README.md`, **Scope and ownership**, **Exact revision keys**, **Computation**, **Transaction**, and **Invalidation** | Compatible | Analyses are immutable exact-revision consumers keyed only by stable IDs; dense indices/pointers/names are local conveniences; mutation creates a new key, stale handles reject, failure publishes no result, and no analysis can mint a stage token or repair core facts. |
| `src/backend/bir/diagnostics/README.md`, **Borrow and revision**, **Structured diagnostics**, **Deterministic rendering**, and **Failure and ownership** | Compatible | Diagnostics/rendering borrow immutable exact-revision views, are read-only presentation only, cannot change success/severity/order or construct/publish stages, and reject stale keys/text feedback. |
| `src/backend/bir/compatibility/README.md`, **Quarantine boundary** | Authority-compatible, wording mismatch | Correctly forbids quarantine input to import/verification/passes and forbids names/text/pointers from creating facts or converting failure to success. However “A missing typed fact is a producer gap” is unscoped and conflicts when applied to phase A: current-LIR facts are complete and missing receipt is a container/importer/verifier gap. This wording must explicitly defer to the owning current-LIR matrix disposition. |
| `src/backend/bir/LEGACY_COVERAGE.md`, `lir_to_bir` coverage row and coverage gate | Authority-compatible, wording mismatch | Correctly assigns typed import/error transport to the top-level importer and rejects text recovery/partial publication, but says “missing structured source carriers remain explicit source failures.” For Child A rows, this must become the exact accepted receiving-side disposition; legacy evidence cannot reopen LIR. |
| `src/backend/bir/REVIEW_TEMPLATE.md`, **Complete boundary walk**, **Ownership and observation**, **Legacy disposition**, and **Same-family proof** | Compatible | Requires exact owner/revision/verifier/failure/consumer evidence, atomic full Raw publication, read-only consumers, no name/text/cache authority, no partial publication, one legacy disposition and neighboring anti-overfit proof. |
| `src/backend/bir/README.md`, stage table A1/A2/B1, publication/invalidation and cross-cutting sections | Stage-compatible; separate stale lifecycle mismatch | A1 private draft, A2 verified target-independent/unallocated Raw, B1 legalize, transactional failure, analysis invalidation and cross-cutting read-only/quarantine roles agree. The introduction still says idea 731 “remains open”; the actual bounded transport idea is under `ideas/closed/731_*` and the active umbrella already records it closed. Root lifecycle/status text needs coordinated reconciliation, but it does not change the A2 semantic result above. |
| `src/backend/bir/passes/legalize/README.md`, **Exact input checkpoint**, **Closed authority**, **Transaction**, and **Unsupported and failure behavior** | Input/authority-compatible, wording mismatch | Accepts only the immutable exact-revision `RawBir` already accepted by the complete Raw profile; relies on stable IDs/def-use/CFG, has no target input, cannot repair malformed input and fails atomically. “Other producer gaps are Raw publication failures” must be scoped to upstream A2 contract failures rather than suggesting a current-LIR producer repair. |

### Exact coordinated documentation boundary blocker

Step 4 is blocked on one coordinated **phase-A source/target-context
reconciliation boundary**. It must name both sides and authorize the shared
files explicitly:

- Accepted phase-A side (comparison authority; no semantic change expected):
  `src/backend/bir/lir_to_bir/README.md` sections **Purpose**, **Exhaustive
  Metadata-Family Input Matrix / module-context**, **Target and ABI Rules**;
  `src/backend/bir/core/README.md` sections **Does Not Own**, **Exhaustive
  Metadata-Family Receiving Matrix / module-context**, **Target and ABI Rules**;
  and `ideas/open/735_bir_phase_a_import_raw_document_convergence.md` as durable
  intent.
- Shared side requiring authorized edits:
  `src/backend/bir/verify/README.md` validation phase 2, type-universe
  lowering-environment clause, LIR source-gap ledger and coverage rows;
  `src/backend/bir/compatibility/README.md` producer-gap sentence;
  `src/backend/bir/LEGACY_COVERAGE.md` typed-import source-failure clause;
  and `src/backend/bir/passes/legalize/README.md` upstream producer-gap phrase.
- Required semantic resolution: Raw/Canonical store no `target_profile`, target
  triple, rendered `data_layout`, language-ABI selection or other C1/C2 target
  context as semantic state. The A2 verifier may validate genuinely
  target-independent typed facts that have their own accepted core owner, and
  may retain non-semantic origin/parity evidence, but it cannot parse or import
  target/layout text. C1 independently selects the exact `TargetProfile`; C2
  derives target layout. Every current 38/6/18 fact keeps its accepted
  container/wiring/validation disposition; desired forms absent from current
  LIR are outside this intake, not authorization for LIR/schema changes.
- Required authority resolution: compatibility and legacy text defer to the
  owning matrix and remain observation/evidence only; B1 describes an inherited
  A2 failure and never a producer-repair route. No shared file may duplicate
  importer/core ownership or weaken fail-closed behavior.

A separate root-index reconciliation in the same authorized shared-doc packet
should correct `src/backend/bir/README.md`'s stale idea-731 lifecycle sentence
against `ideas/closed/731_inline_asm_transport_and_regalloc_contract.md` and
the active umbrella's lifecycle reconciliation. That correction is status
truth only and must not alter the phase order or authorize implementation.

## Suggested Next

- Supervisor should delegate an explicitly coordinated documentation packet
  for the blocker above, naming the accepted phase-A comparison owners and the
  five shared files that require text edits. After that packet is accepted,
  rerun this Step-4 audit; advance to Step 5 only when the mismatch rows become
  compatible without duplicate authority.

## Watchouts

- Do not “resolve” the blocker by importing target/profile/layout text into
  Raw, weakening the A2 gate, adding a compatibility exception, or changing
  LIR. The repair is shared documentation ownership/terminology alignment.
- Preserve all compatible shared rules: exact revision, one full publication
  gate, failure atomicity, stale-analysis rejection, read-only diagnostics,
  empty-production quarantine and B1's exact immutable Raw input.
- The root idea-731 sentence is a lifecycle/status correction, not permission
  to reopen 731 or activate deferred idea 734.

## Proof

- Read-only docs audit: no canonical regression log applies and neither
  `test_before.log` nor `test_after.log` was changed.
- Structural existence/link, phase-A clause, compatible shared-rule, exact
  conflict-presence and diff-scope proof:

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

# Remove fenced/inline code before extracting Markdown links so regex examples
# in LEGACY_COVERAGE are not misread as links.
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

importer = docs['src/backend/bir/lir_to_bir/README.md']
memory = docs['src/backend/bir/lir_to_bir/memory/README.md']
core = docs['src/backend/bir/core/README.md']
verify = docs['src/backend/bir/verify/README.md']
analysis = docs['src/backend/bir/analysis/README.md']
diagnostics = docs['src/backend/bir/diagnostics/README.md']
compatibility = docs['src/backend/bir/compatibility/README.md']
legacy = docs['src/backend/bir/LEGACY_COVERAGE.md']
review = docs['src/backend/bir/REVIEW_TEMPLATE.md']
root = docs['src/backend/bir/README.md']
legalize = docs['src/backend/bir/passes/legalize/README.md']

for text in (importer, memory, core):
    assert 'verify_and_publish_raw(ModuleDraft&&)' in text
    assert 'Implementation-Status:' in text
assert 'validation/origin/parity-only non-destination' in importer
assert 'no semantic core owner' in core
assert 'LirInst=38' not in core  # rows, not a status-only count assertion

compatible_rules = [
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
for text, statement in compatible_rules:
    assert statement in text, statement

blocking_conflicts = [
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
for text, statement in blocking_conflicts:
    assert statement in text, statement

changed = {
    line for line in subprocess.check_output(
        ['git', 'diff', '--name-only'], text=True).splitlines() if line
}
assert changed == {'todo.md'}, changed
print(f'PASS docs={len(paths)} links={link_count} compatible_rules='
      f'{len(compatible_rules)} blocking_conflicts={len(blocking_conflicts)} '
      'changed=todo.md step=4')
PY
git diff --check
```

- Result:

```text
PASS docs=11 links=85 compatible_rules=14 blocking_conflicts=9 changed=todo.md step=4
git diff --check: PASS
```
