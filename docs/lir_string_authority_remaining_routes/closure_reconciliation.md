# 812 closure reconciliation

Evidence revision: `d58b8d44c9b64d2005d2b3760a0592b1b47ebd03`.

The following credits are deliberately narrow. “Bounded only” means Step 2 must keep other producers/consumers in the matrix rather than treating a family name as closed.

| Closure | Exact accepted scope credited | Not credited / still relevant evidence |
| --- | --- | --- |
| 759 | Enum-first typed-reference foundation; builtin enum/id semantic queries. | Runtime construction, HIR text producers, parsed call text, extern text, and verifier/BIR compatibility fallbacks are explicitly deferred (`ideas/closed/759...:73-105`). |
| 760 | Closed-set literal constructor migration and named retained dynamic-text boundaries. | No richer typed carrier for dynamic aggregate/vector/struct/function spellings (`760:97-122`). |
| 761 | Structured call args/fixed params and checked `args_str`; structured switch selector mirror. | Raw compatibility/emission fallback, parser/collector use, and unrelated call identity are not family closure (`761:111-127`). |
| 762 | Structured module declaration/type shadows for structs, extern returns, signatures, and globals. | Out-of-scope lowering/type-model and absent-metadata compatibility must be individually traced (`762:116-122`). |
| 763 | Builtin, integer width, named composite, and selected array structure. | `runtime_text` remains for deferred forms; no universal removal or nested-family inference (`763:109-128`). |
| 754 | Five representative aggregate/vector operation rows and their selected insert/extract/shuffle chain. | Recursive type shapes and aggregate owner routes are not implied (`754:520-536`). |
| 811 | Native vector result/use, lane/element, index, and ordered mask carrier publication. | It is prerequisite-only, not any vector operation-row semantic completion (`811:13-45`). |
| 814 | Poison second-vector shape for the existing scalar-to-vector splat carrier. | No shuffle-row semantic completion; depends on the separately bounded mask route (`814:15-40,97-117`). |
| 815 | Zero-initializer splat `mask_lanes` and display-mirror coherence. | No mask-selection/shuffle semantic claim (`815:46-74`). |
| 832 | HIR aggregate-owner function-parameter crash. | No full baseline, truthiness, or broader aggregate-owner completion (`832:66-83`). |
| 833 | Direct-scalar truthiness-LHS producer/verifier relation. | Four aggregate-owner residuals and comparable baseline are outside scope (`833:65-84`). |
| 834 | `lir_owned_type_spec` aggregate key-to-module-owner canonicalization. | Focused proof only; parent 831 owns comparable gate (`834:126-143`). |
| 835 | Durable HIR canonical aggregate-owner identity carrier. | Parent 834 consuming it and return/baseline work were expressly retained (`835:91-105`). |

## Supersession handling

No historical closure is treated as superseding current code merely by title. Current structured mirrors supersede their matching legacy shadow only where the closure says so (notably 761/762); the active consumer still needs a matrix row where it can parse or fall back to that text. The currently open 838--847 architecture/migration records are newer owner evidence, not closure evidence, and are mapped in `existing_owner_map.md`.
