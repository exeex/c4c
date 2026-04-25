# Idea 98 Step 8 Route Review

## Review Base

- Base commit: `ecc018c1` (`[plan] activate idea 98 parser sema structured identity leftovers`)
- Rationale: this is the commit that activated the current idea 98 runbook. Later `[plan]` commits `d44eb249` and `125ac2a3` repaired/decomposed enum and template-parameter prerequisites inside the same active route; they did not replace the activation checkpoint. The delegated packet also asked for activation-through-HEAD review.
- Commits reviewed: 12 (`ecc018c1..HEAD`)
- Report path: `review/98_step8_route_review.md`

## Findings

No blocking findings.

Low: validation is still narrower than a closure-grade acceptance pass. The current `test_before.log` and `test_after.log` are matching and green for the focused parser/sema/template/consteval subset, and no test expectations were changed. Because the reviewed window spans 12 commits across parser metadata, sema mirrors, and consteval binding maps, Step 8 closure should still require a supervisor-selected broader proof before accepting the whole runbook as complete.

## Alignment Review

Parser helper boundary remains aligned. `eval_const_int` now names the TextId keyed table as the parser-owned primary surface while preserving the rendered-name overload as a compatibility bridge for string-only callers ([support.cpp](/workspaces/c4c/src/frontend/parser/impl/support.cpp:632), [support.cpp](/workspaces/c4c/src/frontend/parser/impl/support.cpp:720), [parser_support.hpp](/workspaces/c4c/src/frontend/parser/parser_support.hpp:13)). The rendered struct-tag map remains in the API for `sizeof`, `alignof`, and `offsetof`; the diff does not attempt the out-of-scope `TypeSpec::tag` or struct/tag migration.

Enum metadata and mirrors stay within the Step 3 route. Parser enum parsing stores per-enumerator `TextId` metadata beside rendered names and values without changing rendered enum output ([struct.cpp](/workspaces/c4c/src/frontend/parser/impl/types/struct.cpp:2431), [struct.cpp](/workspaces/c4c/src/frontend/parser/impl/types/struct.cpp:2470)). Sema then dual-writes global/local enum structured mirrors beside legacy maps and keeps lookups legacy-first with comparison only where structured keys exist ([validate.cpp](/workspaces/c4c/src/frontend/sema/validate.cpp:938), [validate.cpp](/workspaces/c4c/src/frontend/sema/validate.cpp:965), [validate.cpp](/workspaces/c4c/src/frontend/sema/validate.cpp:987)).

Template parameter metadata follows the repaired Step 4 plan. Parser declaration parsing carries per-parameter `TextId` arrays parallel to `template_param_names`, uses `kInvalidText` for synthetic names, and keeps rendered names/defaults/pack metadata intact ([declarations.cpp](/workspaces/c4c/src/frontend/parser/impl/declarations.cpp:1740), [declarations.cpp](/workspaces/c4c/src/frontend/parser/impl/declarations.cpp:1830), [declarations.cpp](/workspaces/c4c/src/frontend/parser/impl/declarations.cpp:2058)). Template instantiation copies the metadata pointer from the primary definition without changing HIR-facing rendered template names ([base.cpp](/workspaces/c4c/src/frontend/parser/impl/types/base.cpp:2993)).

Sema NTTP and type-parameter mirrors are advisory rather than behavior-replacing. Local NTTP placeholders use stable keys when parser metadata exists, while rendered local bindings remain authoritative ([validate.cpp](/workspaces/c4c/src/frontend/sema/validate.cpp:83), [validate.cpp](/workspaces/c4c/src/frontend/sema/validate.cpp:1296)). Type-parameter validation records TextId mirrors and compares presence, then falls back to rendered-name behavior when metadata is absent or ambiguous ([validate.cpp](/workspaces/c4c/src/frontend/sema/validate.cpp:1301), [validate.cpp](/workspaces/c4c/src/frontend/sema/validate.cpp:1332), [validate.cpp](/workspaces/c4c/src/frontend/sema/validate.cpp:2395)).

Consteval NTTP and type-binding mirrors are populated without deleting bridge behavior. `bind_consteval_call_env` now fills NTTP text/key maps and type-binding text/key maps when template-parameter metadata exists, but the legacy name maps remain present in the environment ([consteval.cpp](/workspaces/c4c/src/frontend/sema/consteval.cpp:125), [consteval.cpp](/workspaces/c4c/src/frontend/sema/consteval.cpp:151), [consteval.cpp](/workspaces/c4c/src/frontend/sema/consteval.cpp:175), [consteval.cpp](/workspaces/c4c/src/frontend/sema/consteval.cpp:443)). `resolve_type` compares advisory mirrors against the legacy binding rather than changing result selection ([consteval.cpp](/workspaces/c4c/src/frontend/sema/consteval.cpp:86), [consteval.cpp](/workspaces/c4c/src/frontend/sema/consteval.cpp:114)).

No testcase-overfit evidence found. The reviewed diff does not change tests, expectation modes, HIR, codegen, backend lowering, printed-LLVM probes, or named testcase shortcuts. `git diff ecc018c1..HEAD -- tests ...` is empty, and `git diff ecc018c1..HEAD -- src/frontend/hir src/codegen src/backend` is empty.

## Step 8 Recommendation

Plan-alignment judgment: `on track`

Idea-alignment judgment: `matches source idea`

Technical-debt judgment: `watch`

Validation sufficiency: `needs broader proof`

Reviewer recommendation: `narrow next packet`

Step 8 should be a final audit/demotion packet, not an aggressive fallback-removal packet. The remaining legacy rendered-name fallbacks should stay as explicit compatibility/proof paths unless Step 8 identifies a parser/sema-owned fallback with direct parity evidence and no AST, HIR, consteval, diagnostics, codegen, link-name, or unsupported-metadata dependency. If the broader proof stays green and Step 8 records the remaining fallbacks as bridge-required or proof-required, lifecycle demotion/closure is appropriate.

## Step 8 Final Audit

Final demotion decision: no broad fallback demotion is accepted in this packet. The Step 2-7 changes established structured/TextId mirrors and comparison paths, but the remaining rendered-name surfaces still fall into compatibility bridge, proof-required, or downstream-blocked buckets. Removing any of them now would exceed the runbook's "only after direct proof" rule.

### Compatibility Bridges

- Parser const-int string helper: keep. The primary parser-owned overload now uses `TextId` named constants ([support.cpp](/workspaces/c4c/src/frontend/parser/impl/support.cpp:632)), but the rendered-name overload is explicitly documented as a compatibility bridge for string-only callers ([support.cpp](/workspaces/c4c/src/frontend/parser/impl/support.cpp:720), [parser_support.hpp](/workspaces/c4c/src/frontend/parser/parser_support.hpp:23)).
- Parser struct/tag inputs to constant evaluation: keep. `sizeof`, `alignof`, and `offsetof` still flow through rendered `TypeSpec::tag` and the rendered struct map ([support.cpp](/workspaces/c4c/src/frontend/parser/impl/support.cpp:648), [support.cpp](/workspaces/c4c/src/frontend/parser/impl/support.cpp:654), [support.cpp](/workspaces/c4c/src/frontend/parser/impl/support.cpp:658)); this is the explicit type-surface bridge, not dead parser cleanup.
- AST enum and template rendered arrays: keep. Step 3A and Step 4A added `enum_name_text_ids` and `template_param_name_text_ids`, but preserved `enum_names` and `template_param_names` as the AST/HIR/diagnostic/template-spelling bridge ([struct.cpp](/workspaces/c4c/src/frontend/parser/impl/types/struct.cpp:2470), [ast.hpp](/workspaces/c4c/src/frontend/parser/ast.hpp:330), [declarations.cpp](/workspaces/c4c/src/frontend/parser/impl/declarations.cpp:2058)).
- Consteval environment name maps: keep. The environment still exposes legacy type and NTTP maps beside text/key mirrors ([consteval.hpp](/workspaces/c4c/src/frontend/sema/consteval.hpp:147), [consteval.hpp](/workspaces/c4c/src/frontend/sema/consteval.hpp:154)); `bind_consteval_call_env` populates both surfaces when metadata exists ([consteval.cpp](/workspaces/c4c/src/frontend/sema/consteval.cpp:443), [consteval.cpp](/workspaces/c4c/src/frontend/sema/consteval.cpp:501), [consteval.cpp](/workspaces/c4c/src/frontend/sema/consteval.cpp:539)).

### Proof-Required Fallbacks

- Sema enum rendered maps: keep as proof-required. Global and local enum variants now dual-write structured/value mirrors when `TextId` metadata exists ([validate.cpp](/workspaces/c4c/src/frontend/sema/validate.cpp:965), [validate.cpp](/workspaces/c4c/src/frontend/sema/validate.cpp:987)), but the legacy maps remain the behavior surface for rendered callers and require broader mismatch-free proof before demotion.
- Template NTTP validation local bindings: keep as proof-required. Step 4B routes stable parameter keys into `bind_local` when metadata exists ([validate.cpp](/workspaces/c4c/src/frontend/sema/validate.cpp:1285)), but rendered local names remain necessary for TextId-less and string-facing validation paths.
- Template type-parameter name checks: keep as proof-required. The mirror path records `TextId` metadata and compares structured presence, then falls back to rendered-name behavior when structured metadata is absent or ambiguous ([validate.cpp](/workspaces/c4c/src/frontend/sema/validate.cpp:1301), [validate.cpp](/workspaces/c4c/src/frontend/sema/validate.cpp:1321), [validate.cpp](/workspaces/c4c/src/frontend/sema/validate.cpp:1332)).
- Consteval type-binding result selection: keep as proof-required. Text/key maps are populated, but `resolve_type` still compares advisory mirrors and returns the legacy binding result ([consteval.cpp](/workspaces/c4c/src/frontend/sema/consteval.cpp:86), [consteval.cpp](/workspaces/c4c/src/frontend/sema/consteval.cpp:114)).

### Downstream-Blocked Or Unsupported-Metadata Fallbacks

- Template parameters with synthetic, copied, instantiated, or otherwise missing parameter tokens remain rendered-name only. Parser metadata intentionally stores `kInvalidText` for unsupported stable identity cases ([declarations.cpp](/workspaces/c4c/src/frontend/parser/impl/declarations.cpp:1740), [declarations.cpp](/workspaces/c4c/src/frontend/parser/impl/declarations.cpp:1784)); sema mirror writers skip those cases rather than manufacturing identity.
- HIR/type/codegen visible names remain downstream-blocked. Template instantiation copies parameter metadata for sema use, but still preserves rendered template names and type tags for HIR-facing behavior ([base.cpp](/workspaces/c4c/src/frontend/parser/impl/types/base.cpp:2990)); `TypeSpec::tag`, struct layout keys, diagnostics, codegen names, and link names are outside idea 98.

### Final Classification

- Accepted demotions in Step 8: none.
- Compatibility bridges to preserve: parser const-int rendered helper, parser struct/tag constant-eval inputs, AST enum/template rendered arrays, consteval legacy environment maps, diagnostics/rendering/link-facing names.
- Proof-required fallbacks: sema enum rendered maps, template NTTP local rendered bindings, template type-parameter rendered checks, consteval type-binding legacy result selection.
- Downstream-blocked or unsupported-metadata fallbacks: `TypeSpec::tag` and record/type identity, HIR/codegen/link-name surfaces, synthetic or copied template parameters with `kInvalidText`.

Suggested supervisor next action: run broader proof for the accumulated idea 98 parser/sema/template/consteval route, then ask the plan owner for lifecycle closure review if broad proof stays green. Further fallback removal should be a new narrow packet only when it names one directly proven parser/sema-owned fallback and preserves all bridge-required rendered surfaces.
