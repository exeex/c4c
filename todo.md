Status: Active
Source Idea Path: ideas/open/167_whole_codebase_string_authority_final_audit.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Record Audit Artifact and Follow-Up Ideas

# Current Packet

## Just Finished

Step 5 - Record Audit Artifact and Follow-Up Ideas is complete. The Step 1-4
audit packets were consolidated into this reviewer-readable artifact in
canonical `todo.md`. No implementation, test, `plan.md`, source idea,
`ideas/closed/`, or `review/` files were edited.

Evidence source commits/packets:

| Commit | Packet evidence used |
| --- | --- |
| `3fd483734` | Step 1 audit method, classification legend, search families, false-positive rules, and first candidate inventory. |
| `b64711bbc` | Step 2 frontend parser/sema/HIR covered-domain classification and frontend closed-miss proof. |
| `3f4f6643e` | Step 2 LIR/HIR-to-LIR `LinkNameId` and `StructNameId` bridge classification. |
| `674a385a8` | Step 2 backend/BIR/prepared link-visible identity bridge classification. |
| `e46c4a2e9` | Step 3 backend route-local/generated value, label, slot, string-pool, and prealloc-name classification. |
| `7a2d2b220` | Step 3 frontend/midend route-local, generated, display, and bridge classification. |
| `206043d74` | Step 4 retained compatibility bridge inventory and retirement-candidate grouping. |
| `caab94523` | Supervisor packet handoff advancing the audit to Step 5 consolidation. |

Audit method and classification:

- Broad searches intentionally over-collected `std::string` maps, `find(name)`,
  `lookup(name)`, `*_by_name`, `*_name_map`, `mangled_name`, `source_name`,
  `qualified_name`, rendered helpers, and fallback helpers across `src`,
  `tests`, docs, and top-level build metadata, excluding build outputs,
  `ideas/closed/`, and `review/`.
- Classification used the Step 1 legend: `SA` semantic authority, `CB`
  compatibility bridge, `DO` display/output, `DD` diagnostic/debug, `RL`
  route-local identity, `GT` generated temporary name, and `FP` false positive.
- Later packets narrowed the noisy grep inventory by owner/domain and used
  local source inspection plus targeted compile-database symbol queries where
  ambiguity remained.
- The audit rejected grep-count reasoning: strings were only treated as risky
  when they selected semantic objects, recovered from structured lookup misses,
  synchronized route-local structures, or parsed final-output text back into
  production facts.

Covered structured-domain result:

| Domain | Structured authority confirmed | Retained string path classification | Complete structured miss behavior |
| --- | --- | --- | --- |
| Parser support/state | `TextId`, `QualifiedNameKey`, direct `record_def`, typed record metadata, parser binding maps. | Rendered typedef, const-int, and record-tag maps are `CB`; parser debug/current-name fields are `DO/DD/CB`. | `TextId`/record metadata paths fail closed; rendered overloads are only reached when callers choose no-metadata compatibility maps. |
| Sema validate/type-utils | `SemaStructuredNameKey`, scoped keys, `TextId`, structured enum/static-eval carriers. | Legacy rendered globals/functions/enum maps are `CB`; display/type formatting is `DO/DD`. | Structured key/text misses stop rather than reopening rendered lookup. |
| Sema consteval/canonical symbols | Structured template binding keys, `TextId`, owner/index metadata, canonical identities. | Rendered template/type/NTTP mirrors and fallback name bindings are `CB`; ABI/canonical display strings are `DO`. | Complete type/key/text metadata misses return `Miss`; fallback names cover incomplete metadata only. |
| HIR declaration/record lookup | `LinkNameId`, concrete ids, `ModuleDeclLookupKey`, `HirRecordOwnerKey`, owner indexes. | `fn_index`, `global_index`, `struct_defs`, `template_defs`, and rendered specialization keys are `CB/DO` depending on lookup vs dump use. | Covered paths fail closed except explicit compatibility cases: self-consistent rendered names, rendered qualified imports, incomplete owner metadata, or absent owner index. |
| LIR/HIR-to-LIR | `LinkNameId`, `StructNameId`, typed signature/type-ref mirrors, initializer function ids. | Extern raw-name maps, legacy struct shadows, final `signature_text`/`init_text` scans, and reachability scans are `CB`; emitted LLVM spelling is `DO`. | Known `LinkNameId` or declared `StructNameId` misses do not fall back to rendered names; verifier text lookups reject mirror drift. |
| BIR/backend link-visible identity | BIR link-name ids, imported name tables, call/global/initializer id fields, prepared link tables. | Raw BIR/LIR names, runtime/intrinsic invalid-id placeholders, initializer text parsing, and raw symbol maps are `CB/DO`. | Present id mismatches fail validation or lowering; raw lookup remains only for invalid-id/raw producer boundaries. |

Route-local and generated families:

| Family | Classification | Follow-up lane |
| --- | --- | --- |
| LIR SSA values and block labels | `RL/GT`; function-local names generated by `fresh_tmp`/`fresh_lbl`, rendered to LLVM and consumed before BIR ids are assigned. | idea 169. |
| LIR/BIR generated string-pool and private data labels | `GT/CB`; generated addressable data labels are intentionally not `LinkNameId`. | idea 169 or generated-data cleanup inside that lane. |
| BIR values, params, block labels, local slots, and memory-address local bases | `RL/CB`; local validation and lowering still synchronize structures by raw spelling when ids are unavailable. | idea 169. |
| LIR-to-BIR route-local maps | `RL`; compare, aggregate alias, and block lookup maps key by local SSA/block spellings before typed local ids are complete. | idea 169. |
| Prepared/prealloc value, block, slot, stack, phi, and materialized names | `RL/GT/CB`; `ValueNameId`, `BlockLabelId`, and `SlotNameId` are good local ids, but raw fallback maps and generated name strings still coordinate passes. | idea 169. |
| Sema consteval interpreter locals | `RL/CB`; `by_text`/`by_key` are authority, while `by_name` is no-metadata interpreter-local compatibility. | idea 169. |
| HIR `FunctionCtx` locals, params, static locals, labels, hidden locals, and generated `.LBB` display labels | `RL/GT/CB`; ids/text maps carry authority where available, rendered maps fence compatibility or output. | idea 169 for raw local/label/generated-name cleanup; output labels remain display. |
| Register names and ABI storage strings | `DO/RL`; target storage and final allocation/debug payloads, not source/link identity. | No idea 169 action except keeping them separate from value identity cleanup. |

Retained compatibility bridge summary:

| Owner/lane | Bridges retained | Boundary | Recommendation |
| --- | --- | --- | --- |
| Parser support | `resolve_record_type_spec`, `eval_const_int`, `resolve_typedef_chain`, `types_compatible_p` rendered overloads. | No `record_def`, owner key, `TextId`, or typed HIR binding. | Feed idea 168 plus regression guards for closed structured misses. |
| Sema/type-utils/consteval | Rendered enum consts, template/type/NTTP binding mirrors, fallback canonical template names. | No scoped carrier, incomplete binding metadata, or display/ABI generation. | Feed idea 168 for production bridges; guard enum/template complete-miss behavior. |
| HIR module/record/template/specialization | Rendered decl indexes, `struct_defs`, template maps, specialization canonical/display keys. | Explicit rendered compatibility, imports, incomplete owner metadata, no owner index, or dump/display output. | Feed idea 168; keep dump/diagnostic output out of bridge-retirement scope. |
| LIR final-output and struct mirrors | `signature_text`, `init_text`, `type_decls`, legacy signature/type parsing, reachability scans. | Producer-boundary final LLVM text or parity verifier checks, not ordinary lookup fallback. | Feed idea 168 for production scans and mirror retirement; retain printer output as output. |
| LIR/BIR link-symbol fallback | `extern_decl_name_map`, raw function/global symbol maps, invalid-id runtime/intrinsic placeholders. | Missing `LinkNameId`, raw-only imports, or explicit runtime/intrinsic display tokens. | Feed idea 168 for user/extern/global fallback; separate builtin/runtime display tokens if needed. |
| Route-local/generated backend and frontend bridges | LIR/BIR/prealloc local names, generated string constants, HIR locals/labels, consteval `by_name`. | Local function/module/pass namespace before typed local ids are complete. | Feed idea 169, not idea 168. |

False positives and retained non-action strings:

- Diagnostics, debug text, parity mismatch records, AST/HIR/LIR/BIR printers,
  dumps, final assembly/LLVM text, ABI decoration, and canonical display text
  are not bugs unless a caller feeds them back into production lookup.
- Tests that intentionally create stale rendered names or id/name drift are
  regression witnesses, not evidence to weaken expectations.
- Preprocessor/include maps, docs, `.cpp.md` notes, build metadata, and
  variable names matching `name` patterns were classified as `FP` unless they
  crossed into parser/sema/HIR/LIR/BIR/backend identity selection.
- Register names and target ABI storage strings are allocation/output facts,
  not source or link-visible semantic authority.

Semantic-authority leftovers:

- No uncovered production string family needs a new `ideas/open/` initiative
  from this consolidation. The audit found retained compatibility bridges and
  route-local/generated identity work, not a distinct unowned semantic-authority
  string path.
- Remaining production bridge work fits idea 168; route-local/generated
  identity work fits idea 169; closed-miss proof hardening fits the regression
  guard lane.
- No tiny behavior-sensitive bug was discovered during consolidation, so no
  implementation or test edit was made.

Follow-up recommendations by owner and lane:

| Owner | idea 168 bridge-retirement lane | idea 169 route-local/generated lane | Regression guard lane |
| --- | --- | --- | --- |
| Parser | Retire rendered record/typedef/const-int compatibility overloads once callers carry `record_def`, owner keys, `TextId`, or typed HIR bindings. | None. | Add/keep tests proving complete `TextId`/record metadata misses do not recover through rendered maps. |
| Sema | Narrow rendered enum static-eval and template/NTTP/type binding mirrors; keep ABI/display strings output-only. | Retire consteval interpreter local `by_name` where `by_text`/`by_key` metadata is complete. | Guard enum scoped-carrier misses and consteval binding metadata misses. |
| HIR | Retire/narrow `fn_index`, `global_index`, `struct_defs`, `template_defs`, specialization canonical lookup, and no-owner rendered handoffs. | Move `FunctionCtx` locals/params/static locals, labels, hidden locals, and generated local names toward typed local ids. | Guard module declaration, record owner, template binding, and specialization complete-miss behavior. |
| LIR/HIR-to-LIR | Retire `extern_decl_name_map`, final-output scans, discardable reachability by rendered spelling, legacy struct shadows, and raw aggregate/type mirror bridges after typed carriers cover producers. | Replace LIR SSA/block/generated string-pool coordination by typed local/generated ids where later stages synchronize maps. | Guard `LinkNameId` and `StructNameId` parity/mismatch failure. |
| BIR/backend/prealloc | Retire user/extern/global raw symbol fallback and initializer raw symbol parsing when ids are always carried; keep runtime/intrinsic display tokens separate. | Reduce raw BIR value/block/slot/string-pool and prepared value/block/slot/generated-name synchronization around `ValueNameId`, `BlockLabelId`, and `SlotNameId`. | Guard BIR validation id/name mismatch, invalid-id compatibility boundaries, and prepared link-name drift rejection. |

Lifecycle confirmations:

- New `ideas/open/` initiative needed: no.
- Source idea intent changed: no; the audit findings fit the source idea's
  requested inventory, retained-bridge list, and follow-up recommendation
  output.
- `plan.md` remains correct: yes. Step 5 was completed by recording the audit
  artifact in `todo.md`; Step 6 remains the correct final proof and closure
  readiness step.

## Suggested Next

Proceed to Step 6 - Final Proof and Closure Readiness.

Recommended Step 6 packet:

1. Re-run the audit replay searches used by Steps 1-5, with emphasis on
   bridge-boundary patterns and route-local generated-name patterns.
2. No implementation tests are required unless the replay discovers a
   behavior-sensitive issue; otherwise document that the slice is audit-only.
3. Verify ideas 168 and 169 plus the regression-guard lane still cover all
   follow-up recommendations.
4. Record closure readiness, retained risks, and exact proof commands in
   `todo.md`.

## Watchouts

- Do not create a new source idea for the current findings; consolidation found
  no distinct unowned initiative beyond idea 168, idea 169, and regression
  guards.
- Keep idea 168 and idea 169 separate. Bridge retirement is about rendered
  source/link compatibility boundaries; route-local cleanup is about local
  SSA/label/slot/generated-name ids.
- Keep final-output text separate from semantic lookup: `signature_text`,
  `init_text`, ABI/mangled strings, printer output, diagnostics, and parity
  labels are bridge candidates only when production code scans them to recover
  semantic facts.
- Complete structured misses are fenced in the audited core paths, with known
  compatibility exceptions: HIR self-consistent rendered names, rendered
  qualified imports, raw-only LIR functions/externs, invalid-id BIR runtime/
  intrinsic placeholders, and absent-owner-index record layout handoffs.
- The untracked `review/166_compile_time_registry_fencing_route_review.md`
  pre-existed this packet and was not touched.

## Proof

Audit consolidation only. No tests were required because this packet edited only
`todo.md`, made no implementation or test changes, and discovered no
behavior-sensitive issue. `test_after.log` was not updated.

Commands run:

`sed -n '1,240p' /workspaces/c4c/.codex/skills/c4c-executor/SKILL.md`

`git status --short`

`sed -n '1,240p' AGENTS.md`

`sed -n '1,260p' todo.md`

`sed -n '1,260p' plan.md`

`sed -n '1,220p' ideas/open/167_whole_codebase_string_authority_final_audit.md`

`git log --oneline --decorate -n 20 -- plan.md todo.md src tests include ideas/open/167_whole_codebase_string_authority_final_audit.md`

`git show --stat --oneline 3fd483734 -- todo.md`

`git show 3fd483734:todo.md | sed -n '1,240p'`

`git show --stat --oneline b64711bbc -- todo.md`

`git show b64711bbc:todo.md | sed -n '1,260p'`

`git show --stat --oneline 3f4f6643e -- todo.md`

`git show 3f4f6643e:todo.md | sed -n '1,260p'`

`git show --stat --oneline 674a385a8 -- todo.md`

`git show 674a385a8:todo.md | sed -n '1,260p'`

`git show --stat --oneline e46c4a2e9 -- todo.md`

`git show e46c4a2e9:todo.md | sed -n '1,260p'`

`git show --stat --oneline 7a2d2b220 -- todo.md`

`git show 7a2d2b220:todo.md | sed -n '1,260p'`

`git show --stat --oneline 206043d74 -- todo.md`

`git show 206043d74:todo.md | sed -n '1,260p'`

`nl -ba todo.md | sed -n '1,260p'`

`git diff -- todo.md`

`git diff --check -- todo.md`

`git status --short`
