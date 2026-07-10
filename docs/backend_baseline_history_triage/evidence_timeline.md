# Backend Baseline History Evidence Timeline

Generated for Step 1 of `plan.md` on 2026-07-10. Timestamps are filesystem
modification times in UTC unless otherwise noted. Newer evidence wins when
history conflicts.

## Authoritative Current State

- Newest broad baseline log:
  `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`,
  2026-07-10 04:20:43, `34/3397` failed.
- Latest agent log:
  `build/agent_state/agent_logs/agents_codex_iter_1_7261d7.log`,
  2026-07-10 04:31:35.
- Newest representative 657 state:
  `build/agent_state/657_step3_representative_pointer_value_store/summary.md`,
  2026-07-10 04:18:53. This supersedes the older
  `build/agent_state/657_step4_representative_proof/summary.md`,
  2026-07-10 04:12:24, for the `loop-2e.c` representative result.

## Reverse-Chronological Evidence

### Agent Logs

| Timestamp | Path | Notes |
| --- | --- | --- |
| 2026-07-10 04:31:35 | `build/agent_state/agent_logs/agents_codex_iter_1_7261d7.log` | Latest agent log; records the supervisor delegating this Step 1 docs-only packet. |
| 2026-07-10 04:27:19 | `build/agent_state/agent_logs/agents_codex_iter_1_1f20ac.log` | Recent supervisor bootstrap/log context. |
| 2026-07-10 04:21:49 | `build/agent_state/agent_logs/agents_codex_iter_21_844333.log` | Referenced by `plan.md`; contains the preceding backend-proof handoff context. |

### Recent Agent Summaries

| Timestamp | Path | Evidence |
| --- | --- | --- |
| 2026-07-10 04:18:53 | `build/agent_state/657_step3_representative_pointer_value_store/summary.md` | Focused dump and object-runtime contracts passed; representative `loop-2e.c` RV64 object-runtime comparison now passes against clang; backend subset remains red with `32/368` failed. |
| 2026-07-10 04:12:24 | `build/agent_state/657_step4_representative_proof/summary.md` | Older representative proof reported c4c object-runtime compare mismatch for `loop-2e.c`; superseded by the newer 04:18 Step 3 summary. |
| 2026-07-10 04:03:37 | `build/agent_state/657_step2_destination_writeback_facts/summary.md` | Classifies remaining 657 owner as RV64 object lowering for prepared pointer-value stores. |
| 2026-07-10 03:59:43 | `build/agent_state/657_step1_loop_2e_runtime_boundary/summary.md` | First 657 boundary: callee pointer-value store writes to local frame slot instead of old pointer value; backend proof `32/366` failed. |
| 2026-07-10 03:52:16 | `build/agent_state/656_step4_representative_proof/summary.md` | 656 representative now fails closed with `producer_authority_missing_for_register_fan_in_stack_destination`; backend proof `32/366` failed. |
| 2026-07-10 03:42:59 | `build/agent_state/656_step3_producer_surface/summary.md` | Producer/freshness boundary repaired; focused object runtime blocked by register fan-in stack-destination authority. |
| 2026-07-10 03:32:37 | `build/agent_state/656_step3_callee_result_repair/summary.md` | Earlier blocker: missing `%t3` producer fact for callee result path; no implementation patch made in that packet. |
| 2026-07-10 03:25:43 | `build/agent_state/656_step2_callee_result_contract/summary.md` | Focused dump passed; object-runtime contract failed with expected repair coverage. |
| 2026-07-10 03:20:27 | `build/agent_state/656_step1_20140828_runtime_evidence/summary.md` | First downstream owner for 20140828-1.c classified as callee result propagation. |
| 2026-07-10 02:41:04 | `build/agent_state/653_step4a_loop_t23_runtime_probe/summary.md` | `%t23` pointer-source publication/materialization no longer first owner; downstream callee indirect-store writeback remained. |
| 2026-07-10 02:36:56 | `build/agent_state/653_step4a_loop_t23_clobber_safety/summary.md` | `%t23` prepared/RV64 branch stack-load authority no longer owner; backend subset red with `32/365` failed. |
| 2026-07-10 02:26:37 | `build/agent_state/653_step4a_loop_t23_pointer_source_chain/summary.md` | Pointer source chain published; next diagnostic was branch stack-load authority. |

### Baseline Logs

`ls -lt log/baseline_*.log` found 213 baseline logs. The current triage
surface begins at the latest clean log on 2026-07-09 12:47 and then expands
through the current 34-failure plateau:

| Timestamp | Path | Failed/Total | Baseline Subject |
| --- | --- | ---: | --- |
| 2026-07-10 04:20:43 | `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log` | 34/3397 | Apply RV64 pointer-value store lowering to representative shape |
| 2026-07-10 02:28:44 | `log/baseline_b5900d89fd30348c8e79901ce3e5e14875274d03.log` | 34/3394 | Publish loop pointer compare source chain |
| 2026-07-10 00:58:16 | `log/baseline_575142db412ec5e846e77a6abcef8a77d7e97d83.log` | 34/3394 | Publish stack-carried pointer source authority |
| 2026-07-09 22:53:39 | `log/baseline_24c42f8da1ba7099134e97af222391de11ec5753.log` | 32/3389 | support RV64 edge store local publication carriers |
| 2026-07-09 19:35:50 | `log/baseline_11ee2da0f73d9328066ae05b84d75028df1adce7.log` | 31/3376 | close RV64 branch same-block identity idea 646 |
| 2026-07-09 17:57:21 | `log/baseline_8a1ed1685aaa22a36153bc13c7acf08aeda05cd1.log` | 20/3376 | Materialize RV64 local frame address call sources |
| 2026-07-09 15:56:55 | `log/baseline_9ae9ddfce0163d04b7df19e956175a6c8ccb1083.log` | 13/3376 | Publish pr79737 runtime mismatch owner research |
| 2026-07-09 12:47:32 | `log/baseline_0d937a44156ddf33c9df2691925d87e3f24a0978.log` | 0/3376 | Publish branch stack clobber-safety authority |
| 2026-07-09 10:13:20 | `log/baseline_199e5793023c26cce18c2dc6d911ab74cf27eeb8.log` | 0/3376 | Admit RV64 direct-global local memory |
| 2026-07-09 08:11:28 | `log/baseline_d2c2072ad34cbee556d9f838b9a6517f6f1136bf.log` | 0/3376 | Record prepared return destination-home carrier |
| 2026-07-09 06:32:45 | `log/baseline_3fb1df919f15b8c49fcdae4c6a18fb6077e6fcc7.log` | 0/3376 | Publish dynamic GPR callee-saved slot placements |
| 2026-07-09 04:47:47 | `log/baseline_5c476862ddd8ef00a62681e959041680cb2b1d66.log` | 0/3376 | Publish RV64 outgoing stack destination facts |
| 2026-07-09 03:04:04 | `log/baseline_fc1d86c91dd1cc7d29d4d6cbc14b66d603d97d40.log` | 0/3376 | publish stack destination fan-in authority fact |
| 2026-07-09 01:41:25 | `log/baseline_daf5c100261780de73b4c2f9f6d33f0643ee800f.log` | 0/3375 | Write runtime symptom map |
| 2026-07-08 22:40:36 | `log/baseline_d1e6f28bcf3d0c4fe164d9c16ddfce884e39588a.log` | 0/3375 | Allow duplicate prepared frame address call facts |
| 2026-07-08 19:11:17 | `log/baseline_cd470040edb572b63ee74b7e1c42dbecdcd2ed85.log` | 0/3375 | Publish byte-storage global layout authority |
| 2026-07-08 17:30:12 | `log/baseline_ee3b1c5559bb0d9589be0e59b637eba9f32312d2.log` | 5/3375 | populate mixed object-data relocation slots |
| 2026-07-08 15:37:00 | `log/baseline_3c770af993cc7b721e1c3bf56c97ad02ede639df.log` | 0/3375 | close BIR global initializer bootstrap idea |

Older history contains alternating one-off clean/red points before the current
triage surface. Step 2 should classify the current 2026-07-10 04:20 row set
first, not historical rows that were later returned to green.

### Complete Baseline Log Inventory

Full `log/baseline_*.log` inventory from `ls -1t` order:

| Timestamp | Path | Failed/Total |
| --- | --- | ---: |
| 2026-07-10 04:20:43 | `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log` | 34/3397 |
| 2026-07-10 02:28:44 | `log/baseline_b5900d89fd30348c8e79901ce3e5e14875274d03.log` | 34/3394 |
| 2026-07-10 00:58:16 | `log/baseline_575142db412ec5e846e77a6abcef8a77d7e97d83.log` | 34/3394 |
| 2026-07-09 22:53:39 | `log/baseline_24c42f8da1ba7099134e97af222391de11ec5753.log` | 32/3389 |
| 2026-07-09 19:35:50 | `log/baseline_11ee2da0f73d9328066ae05b84d75028df1adce7.log` | 31/3376 |
| 2026-07-09 17:57:21 | `log/baseline_8a1ed1685aaa22a36153bc13c7acf08aeda05cd1.log` | 20/3376 |
| 2026-07-09 15:56:55 | `log/baseline_9ae9ddfce0163d04b7df19e956175a6c8ccb1083.log` | 13/3376 |
| 2026-07-09 12:47:32 | `log/baseline_0d937a44156ddf33c9df2691925d87e3f24a0978.log` | 0/3376 |
| 2026-07-09 10:13:20 | `log/baseline_199e5793023c26cce18c2dc6d911ab74cf27eeb8.log` | 0/3376 |
| 2026-07-09 08:11:28 | `log/baseline_d2c2072ad34cbee556d9f838b9a6517f6f1136bf.log` | 0/3376 |
| 2026-07-09 06:32:45 | `log/baseline_3fb1df919f15b8c49fcdae4c6a18fb6077e6fcc7.log` | 0/3376 |
| 2026-07-09 04:47:47 | `log/baseline_5c476862ddd8ef00a62681e959041680cb2b1d66.log` | 0/3376 |
| 2026-07-09 03:04:04 | `log/baseline_fc1d86c91dd1cc7d29d4d6cbc14b66d603d97d40.log` | 0/3376 |
| 2026-07-09 01:41:25 | `log/baseline_daf5c100261780de73b4c2f9f6d33f0643ee800f.log` | 0/3375 |
| 2026-07-08 22:40:36 | `log/baseline_d1e6f28bcf3d0c4fe164d9c16ddfce884e39588a.log` | 0/3375 |
| 2026-07-08 19:11:17 | `log/baseline_cd470040edb572b63ee74b7e1c42dbecdcd2ed85.log` | 0/3375 |
| 2026-07-08 17:30:12 | `log/baseline_ee3b1c5559bb0d9589be0e59b637eba9f32312d2.log` | 5/3375 |
| 2026-07-08 15:37:00 | `log/baseline_3c770af993cc7b721e1c3bf56c97ad02ede639df.log` | 0/3375 |
| 2026-07-08 12:00:04 | `log/baseline_cdd3ff0bd4f7a4b2ab7c6cfc019614d54c7d6c20.log` | 0/3375 |
| 2026-07-08 11:55:41 | `log/baseline_7c516c6d619bc9147f6812f8a84ce63ceea42d14.log` | 0/3375 |
| 2026-07-08 11:45:10 | `log/baseline_f68f41fd86f2da46a326a88ffe12a89a9b1fc32f.log` | 0/3375 |
| 2026-07-08 11:41:52 | `log/baseline_d83b005c47b6a83178e6e26891a83d6a66d4951f.log` | 0/3375 |
| 2026-07-08 11:34:18 | `log/baseline_223311fcdcdfb5a7fc8bf84c1520aad4d389d7fb.log` | 0/3375 |
| 2026-07-08 10:54:27 | `log/baseline_50410e1a9689980b32cefb7c4a2283ba7dbd9cc2.log` | 0/3375 |
| 2026-07-08 10:49:09 | `log/baseline_acc3c0b6e2f17233ebe1f6559ebc758f3c02022e.log` | 0/3375 |
| 2026-07-08 10:09:15 | `log/baseline_bf413a92a8e47b2128afa34935b4d90b5079c58f.log` | 0/3375 |
| 2026-07-08 10:06:22 | `log/baseline_db6a6f0ec6ea67c22757615b9dc55acee11178a5.log` | 0/3375 |
| 2026-07-08 10:02:10 | `log/baseline_5c04b7fb85bbf8198c808705c2a8ef845d14db4f.log` | 0/3375 |
| 2026-07-08 09:35:50 | `log/baseline_9a1db3ea0eba88a2e8bc296ee599bbdb2cf7848a.log` | 0/3375 |
| 2026-07-08 09:30:27 | `log/baseline_bc3ccad534a8e4d04b438ac25b92fd6b9581f7bb.log` | 0/3375 |
| 2026-07-08 08:57:39 | `log/baseline_b68af4dc6742693cec4f442a8d15690fc651b1a6.log` | 0/3375 |
| 2026-07-08 08:51:48 | `log/baseline_a0d5d14386300c7d76404cb6aa773dee1f55df20.log` | 0/3375 |
| 2026-07-08 08:46:48 | `log/baseline_5921285e1a8306ac65e64116247c139a9e7006ea.log` | 0/3375 |
| 2026-07-08 08:41:10 | `log/baseline_63ae57c2343de444d1e722b21b9bb14ef8a12d61.log` | 0/3375 |
| 2026-07-08 08:34:29 | `log/baseline_e94d90bfeb40a2d84312acd7ed539a3b19f75b48.log` | 0/3375 |
| 2026-07-08 08:22:38 | `log/baseline_0e16764681e77f6dc2d7e4e5719dbd6954166f2d.log` | 0/3375 |
| 2026-07-08 08:18:21 | `log/baseline_481d77600e9e8e5fb78ffa1f2dbf39acda290166.log` | 0/3375 |
| 2026-07-08 08:08:32 | `log/baseline_a7ddde18cd0eaf46a23ba17871c273ec8a2c1f42.log` | 0/3375 |
| 2026-07-08 08:04:01 | `log/baseline_9101e19dc8274f42ad3e5b7c36dead99fa5a5575.log` | 0/3375 |
| 2026-07-08 07:59:19 | `log/baseline_1ad1193793daef1b06b25346be2e9136cc145198.log` | 0/3375 |
| 2026-07-08 07:42:48 | `log/baseline_7ce1b49b0e0b30252c8b2a69a17581dc3e9bed4f.log` | 0/3375 |
| 2026-07-08 07:36:48 | `log/baseline_e81b5e0ae1d7b11339621eab0d4740ee3736157a.log` | 0/3375 |
| 2026-07-08 07:19:21 | `log/baseline_9f90531a59d7b1f7d6f35667ba5185843d39d5c5.log` | 0/3375 |
| 2026-07-08 07:15:22 | `log/baseline_fcd9277fd5810169142d131f7e889f7846563086.log` | 0/3375 |
| 2026-07-08 06:50:41 | `log/baseline_cb011e5df39936c76af8f4e8d460d0068708bbcf.log` | 0/3375 |
| 2026-07-08 06:38:46 | `log/baseline_3ff9b104747d61d426e47acc3c086baa4fc47197.log` | 0/3375 |
| 2026-07-08 06:32:47 | `log/baseline_d1780581c40cfc0272a606815b31cbff9983edce.log` | 0/3375 |
| 2026-07-08 06:16:42 | `log/baseline_14870fc89d6fdf0b41267316033ed91b31188b7b.log` | 0/3375 |
| 2026-07-08 06:11:27 | `log/baseline_5f4e523f2a1779e0d83686bcff9c235cd64490d7.log` | 0/3375 |
| 2026-07-08 06:07:03 | `log/baseline_1e5770e1f1d080ada0a681d157ec9b64495dfb24.log` | 0/3375 |
| 2026-07-08 06:01:04 | `log/baseline_40ce3353ce575a5e4908e02748754d830c3614c1.log` | 0/3375 |
| 2026-07-08 05:40:28 | `log/baseline_7ada2a3ece275b728965a881ad2facca3e3e8df0.log` | 0/3375 |
| 2026-07-08 05:34:19 | `log/baseline_185de18c907af0e7ca93df06e79921cd6fec8d0d.log` | 0/3375 |
| 2026-07-08 05:21:51 | `log/baseline_8bac56c73d1418a75b7c7f52d33018dac2c6ec0d.log` | 0/3375 |
| 2026-07-08 05:02:46 | `log/baseline_6ec92b5c49d71915118881b88e914d458c8be968.log` | 0/3375 |
| 2026-07-08 04:57:21 | `log/baseline_080028da084410b12ad522b1a064016134c9c189.log` | 0/3375 |
| 2026-07-08 04:48:59 | `log/baseline_df5839dbd5333899df4f35e48adac719f59e5ce5.log` | 0/3375 |
| 2026-07-08 04:38:33 | `log/baseline_d7f3092b99b898b9cdbfc5f37bf9be62a73b744e.log` | 0/3375 |
| 2026-07-08 04:30:43 | `log/baseline_6801046c7ec1a80899fc5961fe9a828850cc7e80.log` | 0/3375 |
| 2026-07-08 03:50:48 | `log/baseline_2aa7fd3a7c564da7d6e26908d5c8b9a8663f7a14.log` | 1/3375 |
| 2026-07-08 03:44:41 | `log/baseline_359519e499ca23c22220296228b5416aefa38517.log` | 1/3375 |
| 2026-07-08 03:40:20 | `log/baseline_6b74484c5cbf717a31e1238574b80a6071f0caab.log` | 1/3375 |
| 2026-07-08 03:31:00 | `log/baseline_b2c70e92c9341263bc1fb6a31d91739a59a0fe8d.log` | 1/3375 |
| 2026-07-08 03:08:57 | `log/baseline_69ff62b0e0f90c016d16855d13ad2a5e0668f866.log` | 1/3375 |
| 2026-07-08 03:01:43 | `log/baseline_474e04bd2f799f3092ceb5dc02e55869c119fa15.log` | 1/3375 |
| 2026-07-08 02:53:28 | `log/baseline_a4d619455b35f89aa8d44b758ea3b314fe161e64.log` | 1/3375 |
| 2026-07-08 02:22:43 | `log/baseline_8920d6a9799e84582eccf4c6f2be6389046de583.log` | 1/3375 |
| 2026-07-08 02:16:14 | `log/baseline_d76b327fbc566d141bf0bb391c92dbca19955d9e.log` | 1/3375 |
| 2026-07-08 02:04:44 | `log/baseline_d9e35d7c26f370e06a7906834714e243113697f0.log` | 1/3375 |
| 2026-07-08 01:33:08 | `log/baseline_51b982a08a301d1ad3a56522d757dc37ee057562.log` | 1/3375 |
| 2026-07-07 18:13:37 | `log/baseline_8d8b37847a5075ffe25dc3cb3ce53aaaea6281de.log` | 1/3375 |
| 2026-07-07 17:21:43 | `log/baseline_fbeba816c190a95a49c2ead99355a984ba8c4c52.log` | 1/3375 |
| 2026-07-07 15:41:38 | `log/baseline_4a59907f4bd6b59fa92a8e22e03ce6b9a8fef4b0.log` | 1/3375 |
| 2026-07-07 15:37:57 | `log/baseline_c9f115e4393079070220417d528abda4b3d35349.log` | 1/3375 |
| 2026-07-07 15:25:15 | `log/baseline_c7aa10f2dbc048d598917e36e383b3496f92e9ca.log` | 1/3375 |
| 2026-07-07 15:21:32 | `log/baseline_038a00fc410801ec40a3cdfb620f60e14f62072f.log` | 1/3375 |
| 2026-07-07 14:47:23 | `log/baseline_3c5e2b350fc1a335241dbf542cfc0e6219697595.log` | 1/3375 |
| 2026-07-07 13:50:01 | `log/baseline_04f39fa61029d01d16ee306239b910575e640114.log` | 1/3375 |
| 2026-07-07 13:25:55 | `log/baseline_5c196ab24b87f7502dc82f3c966ce3f109b87545.log` | 1/3375 |
| 2026-07-07 13:12:11 | `log/baseline_6c8901f226199aba3d3454113f6f7ea6b095af63.log` | 1/3375 |
| 2026-07-07 12:33:13 | `log/baseline_1b133380272430bfffd48bedb27df76f8057c4b9.log` | 1/3375 |
| 2026-07-07 11:55:17 | `log/baseline_6a7cb3eba3da30c31c3ae92714755dad123d3a90.log` | 1/3375 |
| 2026-07-07 11:32:28 | `log/baseline_6bf3565df7199965950fa3c49b2144e71cc48473.log` | 1/3375 |
| 2026-07-07 10:59:35 | `log/baseline_0b05eed5b4984b23aa24cca65e87fc160c0cbd99.log` | 1/3375 |
| 2026-07-07 10:50:09 | `log/baseline_b1a3bfd76b29ae988383dfb43683d5645611c7e5.log` | 1/3375 |
| 2026-07-07 10:42:51 | `log/baseline_c0007715d0d3897ddde833ee24d96bd8b3ebd772.log` | 1/3375 |
| 2026-07-07 10:31:41 | `log/baseline_d532f8b72e80adccfc04d5f8e90d0c84d5003a03.log` | 1/3375 |
| 2026-07-07 10:03:50 | `log/baseline_bfcac2bcfbe7001c4b17c2b2450d4a6d549cc546.log` | 1/3375 |
| 2026-07-07 09:54:51 | `log/baseline_e7411fd15375f1975d759791ff774ecba7449ee0.log` | 1/3375 |
| 2026-07-07 08:45:57 | `log/baseline_646169e184c48d331bb224c3eca57755ea16fb6f.log` | 1/3375 |
| 2026-07-07 03:52:32 | `log/baseline_5e094629375e0208ff0535bbc2183da0ea0a3ff8.log` | 1/3375 |
| 2026-07-03 20:23:17 | `log/baseline_1075e71e6d4932b6d15fbf8974bd43f1efd75576.log` | 1/3375 |
| 2026-07-03 19:51:45 | `log/baseline_6e6dd08562fb5266183ad894ecac9148f64571c2.log` | 1/3375 |
| 2026-07-03 19:32:19 | `log/baseline_b94d20e512f8c9202c1b83c75ded2e12da589d0f.log` | 1/3375 |
| 2026-07-03 19:27:51 | `log/baseline_49b0763d2cd58ac770d95162e15bf0fd55cdc8ca.log` | 1/3375 |
| 2026-07-03 18:40:52 | `log/baseline_866d0c31458217c43a88b7ee7a03de20c6a4f452.log` | 1/3375 |
| 2026-07-03 18:22:45 | `log/baseline_4a04bf83719e2af6d4d94426927842a23d28403b.log` | 1/3375 |
| 2026-07-03 18:09:30 | `log/baseline_afd9cf521cb28394a45d5c622b0e1c64dcac8a2b.log` | 1/3375 |
| 2026-07-03 18:06:46 | `log/baseline_e8b452c72834358fc9eaa0bf0bdd3013f28c0e6c.log` | 1/3375 |
| 2026-07-03 17:52:42 | `log/baseline_35454ca0c779e9f6ceb771e75006c117a15e01bf.log` | 1/3375 |
| 2026-07-03 17:36:58 | `log/baseline_ca599a957ccbfe1f8feece1137565a7b45b55ec8.log` | 1/3375 |
| 2026-07-03 04:44:10 | `log/baseline_34e0da2afc5eb66459889c5f087dd1f50a8708e5.log` | 1/3375 |
| 2026-07-03 01:25:57 | `log/baseline_d5766079426026e8d62d083118354ee2094f2e60.log` | 1/3374 |
| 2026-07-02 23:00:46 | `log/baseline_f5135d411f2b7d86787cae3ff92b52407e4ca399.log` | 1/3374 |
| 2026-07-02 21:40:48 | `log/baseline_61735dfafdf1321e5b8dc2b4a241229de32fcfd9.log` | 1/3374 |
| 2026-07-02 19:54:50 | `log/baseline_4cd89a9c82df6d317d71b54757d0f91fdef7b59c.log` | 0/3374 |
| 2026-07-02 18:04:16 | `log/baseline_40d6045c07975af74882f73b1993dac4932da8ed.log` | 0/3374 |
| 2026-07-02 17:17:03 | `log/baseline_c1a8c912e3bcfdb546ac7e48002f520a3cb8123c.log` | 0/3374 |
| 2026-07-02 16:17:34 | `log/baseline_ddc3386ad4b2fffbed4cd6637e57ef61a7c88ba7.log` | 0/3374 |
| 2026-07-02 15:39:02 | `log/baseline_f65d7d587753a000b6d4c1b23390e5931f1ff293.log` | 0/3374 |
| 2026-07-02 14:15:43 | `log/baseline_035ef072e699a9d963b46a8653a569eebe077ccd.log` | 1/3374 |
| 2026-07-02 13:19:30 | `log/baseline_ad43bb4b11262caea504f1b8bdc81b9b92bc0a96.log` | 1/3374 |
| 2026-07-02 12:01:28 | `log/baseline_974fa04233189128da7f67ec595bd097275babe2.log` | 1/3374 |
| 2026-07-02 10:50:53 | `log/baseline_389f002d7afef27aa5ee4175f1c9563ecd57ce4f.log` | 1/3374 |
| 2026-07-02 09:38:35 | `log/baseline_779d4f0593792289f545889058adf7ba5a120164.log` | 1/3374 |
| 2026-07-02 08:01:43 | `log/baseline_9fc4e220515d1199e47c5934c19bb98ad12e9bbe.log` | 1/3374 |
| 2026-07-02 06:17:29 | `log/baseline_e8a63f4df7061d9fc3b8bee47f0be197faf98b9b.log` | 1/3374 |
| 2026-07-02 04:23:34 | `log/baseline_d7e65524a24351e59fd87a7f8f7872535f0c03f9.log` | 1/3374 |
| 2026-07-02 03:48:58 | `log/baseline_1c0f3f93d83312ab578bc2e3468c9d1dfcb62c22.log` | 1/3374 |
| 2026-07-02 01:51:50 | `log/baseline_97491671576eb83c7e27f33a6fc054f5c89d1706.log` | 1/3374 |
| 2026-07-02 00:14:23 | `log/baseline_96f02d9653ae9f76a8a59c0bd06f6205da15f61c.log` | 1/3374 |
| 2026-07-01 21:58:44 | `log/baseline_f8ec0a640fc37809186f8ff340279357d6614481.log` | 1/3368 |
| 2026-07-01 20:21:22 | `log/baseline_d6feec796871daa7076d02744c4e9aa4a4347505.log` | 1/3361 |
| 2026-07-01 18:39:41 | `log/baseline_772117092529d6e3a2c650e7a6463d4679331220.log` | 2/3359 |
| 2026-07-01 17:37:15 | `log/baseline_444f06fd83bfc2e0dc2d512836356346dec32e49.log` | 1/3357 |
| 2026-07-01 16:29:02 | `log/baseline_c4a485f5dc332a4b57e0eae17af3add4a6aa4eaf.log` | 1/3357 |
| 2026-07-01 14:24:00 | `log/baseline_28970e9142398e2a5e5bd199466bd94108f7d6d0.log` | 1/3357 |
| 2026-07-01 13:00:33 | `log/baseline_fbb2bff5aaa803fb6c6c6541aedd39a46aae61f9.log` | 1/3357 |
| 2026-07-01 10:43:45 | `log/baseline_46391a1829b9ca39e3f80246266d0fd26ceaa916.log` | 1/3357 |
| 2026-07-01 08:30:01 | `log/baseline_c59a4b57e79c5da6a766890a9c31448eb2cdc293.log` | 1/3357 |
| 2026-07-01 06:41:50 | `log/baseline_9680a9509bab8b7a73ec474f6b873caa3a800c18.log` | 3/3357 |
| 2026-07-01 02:10:25 | `log/baseline_c91af210a4612a52255500ffb16ca28559c95cf6.log` | 1/3357 |
| 2026-06-30 21:15:29 | `log/baseline_d6e16836021e8d5fd2991e61b9291250b6c0d31f.log` | 1/3356 |
| 2026-06-30 16:37:28 | `log/baseline_ad79a18e1f9111b6ac1101b3161a0166e3b2c5b9.log` | 1/3356 |
| 2026-06-30 12:49:00 | `log/baseline_a2553075a4206c37f198372f6a194565fe1603c6.log` | 1/3356 |
| 2026-06-30 10:09:05 | `log/baseline_64ff6ae71dba8c58d954f01b4252e23de2153b64.log` | 1/3356 |
| 2026-06-30 07:41:29 | `log/baseline_17af03dcf320f5dabe083536e16d1c34c4f3b9ea.log` | 0/3356 |
| 2026-06-30 06:04:09 | `log/baseline_21410ed27bf9fe752455dd993fdd0569defac7b8.log` | 0/3356 |
| 2026-06-30 03:52:56 | `log/baseline_8a07e2d14eb321aab2a1be757a2729a3e7b933bb.log` | 0/3356 |
| 2026-06-30 01:19:59 | `log/baseline_a3b9bbd66292770bf6975a042e2eb97ff1ad2d7c.log` | 1/3369 |
| 2026-06-29 21:40:42 | `log/baseline_f7c4ec6b6afafc208295cea80bb6a7c457d5a399.log` | 1/3369 |
| 2026-06-29 17:20:58 | `log/baseline_017890e272edfc5c016f40779ecbaa0e3a40bd40.log` | 1/3369 |
| 2026-06-29 14:19:20 | `log/baseline_02c930551268723d538c30659848626b43c798d6.log` | 1/3369 |
| 2026-06-29 12:31:25 | `log/baseline_0c82249f5c8a2f201d33af52137b967378afabbc.log` | 1/3369 |
| 2026-06-29 09:02:26 | `log/baseline_6218e0d43997ae3d4ca57719f745a7d97ce0f7e7.log` | 1/3369 |
| 2026-06-29 05:26:06 | `log/baseline_21a948e9b01f88232dc8f551f6456b17e2cb94b4.log` | 1/3369 |
| 2026-06-29 01:27:34 | `log/baseline_52faaba9ac4c6d9d58100e9a8d30391b622094ab.log` | 1/3369 |
| 2026-06-28 22:23:27 | `log/baseline_22bbe64311e13f55fafd54affcd821de5eedaaa7.log` | 1/3369 |
| 2026-06-28 19:39:54 | `log/baseline_b981f0004dafd3b5d66a98a587f28e0c53920531.log` | 0/3369 |
| 2026-06-28 14:53:38 | `log/baseline_4188acbf247435702a8c7901e33b7aa85c93d381.log` | 0/3368 |
| 2026-06-28 13:14:03 | `log/baseline_dd32fccd7c1438df69d6cf19feb6ae54212ae187.log` | 0/3368 |
| 2026-06-28 06:14:37 | `log/baseline_ffc479891aba85b15c12b37c15932d45698422c9.log` | 0/3368 |
| 2026-06-28 04:17:52 | `log/baseline_acc22d53df5a02878cb0dc9cda84ccfdb60d51f6.log` | 0/3368 |
| 2026-06-28 00:15:21 | `log/baseline_ba88d83c06958fe28b2abd228164490e69d7eaa4.log` | 0/3368 |
| 2026-06-27 23:08:43 | `log/baseline_9d0f64883f4969c32a69e2b869a2dc55d8e72f23.log` | 0/3368 |
| 2026-06-27 20:07:21 | `log/baseline_b6e31e6bdd4c841a27f87238fe91a2e7b90edaca.log` | 0/3366 |
| 2026-06-27 18:56:14 | `log/baseline_1726fa711f9bda61dc6db71df8c13fb7b8b9b33b.log` | 0/3364 |
| 2026-06-27 17:50:51 | `log/baseline_f5dc0acb551ae0ca03e7517373576e59f7c9de85.log` | 0/3356 |
| 2026-06-27 16:43:33 | `log/baseline_4d029b0b6e834b91c18d72b6d3c1967073c18aee.log` | 0/3356 |
| 2026-06-27 13:00:36 | `log/baseline_1ce0086d0de82d1ac6ddd04f535c18cd2b59f869.log` | 0/3356 |
| 2026-06-27 11:55:26 | `log/baseline_daf04bb7528c24cfff86caa4d4e47c3fd4602319.log` | 0/3356 |
| 2026-06-27 11:03:37 | `log/baseline_6beadcbcdd2001760923c7fbfeb65f722ed8fa99.log` | 0/3356 |
| 2026-06-27 10:09:35 | `log/baseline_36555af1db64674575b23bf341741946343002f3.log` | 0/3356 |
| 2026-06-27 09:02:47 | `log/baseline_7d3f8d784ae34aebebeb6b2211107183cb302ccb.log` | 0/3356 |
| 2026-06-27 07:50:23 | `log/baseline_50edb7f46a202382b0f0aaf305c6f6975d612cfa.log` | 0/3355 |
| 2026-06-27 00:29:59 | `log/baseline_09f79ee8dc2ecdde7833c1f6f41b589dabd91c7a.log` | 0/3355 |
| 2026-06-26 21:35:48 | `log/baseline_2d1c6d684c729f1c2052d6efec03039d8c3915e1.log` | 0/3355 |
| 2026-06-26 19:23:58 | `log/baseline_14dc4dcd3a2ea41327e217a52fdfb3ff1a8243ea.log` | 0/3355 |
| 2026-06-26 19:07:29 | `log/baseline_f847dd208e3f2532088bdd26639e3f9a8c394af9.log` | 0/3355 |
| 2026-06-26 18:20:50 | `log/baseline_e2d64441f207d3bb0c50fc57d9fc0e78a174c73c.log` | 0/3355 |
| 2026-06-26 17:57:54 | `log/baseline_0792f5f953357aadee62afc3f7abb8993f861099.log` | 0/3355 |
| 2026-06-26 12:08:35 | `log/baseline_95019debba5b2422e4807918b17f134e39bb4587.log` | 0/3355 |
| 2026-06-26 09:00:03 | `log/baseline_323b42720a2f13998b61b02da71fb4b6876d5688.log` | 0/3355 |
| 2026-06-26 06:46:52 | `log/baseline_ab05dc368e846e54cf6b01282f564c1ec91fd527.log` | 9/3353 |
| 2026-06-26 04:55:08 | `log/baseline_1bc45813aeb02e5d133950045ca01c31282adc57.log` | 2/3353 |
| 2026-06-26 02:08:05 | `log/baseline_a2f04e9ed67ef2e6fa2628d6461388dc8b68be56.log` | 0/3353 |
| 2026-06-25 23:24:37 | `log/baseline_7a775c78eab77560478e15e75b86d7d3a1f71b34.log` | 0/3353 |
| 2026-06-25 21:43:46 | `log/baseline_ac43eeb74d788ae59eed3c0fe387dd1569c08845.log` | 0/3353 |
| 2026-06-25 20:40:00 | `log/baseline_90148a01042ae248bd9ad09957408382158fb706.log` | 1/3353 |
| 2026-06-25 18:56:30 | `log/baseline_8278eed37ed3ba1ce66eab135108492c44b03d8f.log` | 1/3353 |
| 2026-06-25 16:31:23 | `log/baseline_9553a9c68fd14df260ca1b86ec465eb79e254a36.log` | 2/3352 |
| 2026-06-25 15:12:50 | `log/baseline_ec1dfbf3f4847720492218c83e1439fa4b546cee.log` | 0/3352 |
| 2026-06-25 12:55:44 | `log/baseline_44c866bee6cf3b3b577a08af456d55a0674ee2d7.log` | 0/3352 |
| 2026-06-25 11:59:58 | `log/baseline_b1889cc95b329a12de0ca570abad2895e35f6db4.log` | 0/3352 |
| 2026-06-25 08:54:20 | `log/baseline_84503bad13d02c40d12adae011165374bd1839be.log` | 1/3351 |
| 2026-06-25 07:14:44 | `log/baseline_f4001e809099f78fff1422b47a9ceb13a1012d64.log` | 0/3351 |
| 2026-06-25 06:32:29 | `log/baseline_e7f4d8c375fc68f7850576910485768532e0aa3e.log` | 0/3349 |
| 2026-06-25 05:54:07 | `log/baseline_9bede88feba285d3d7d65f27feba82b30f61b989.log` | 0/3348 |
| 2026-06-24 18:38:01 | `log/baseline_b8bb0ca9aabceb05b8244136ff3687013b0a5abe.log` | 0/3346 |
| 2026-06-24 17:26:11 | `log/baseline_cad43111e89f360fbf31222de46c944aabd25e04.log` | 0/3345 |
| 2026-06-24 13:28:56 | `log/baseline_a58fcc03b51bc31f0e6908a308ccf7c7b93a82aa.log` | 1/3345 |
| 2026-06-24 12:19:43 | `log/baseline_9fddf5df6d97067158dedb2a9677b605ed5bde26.log` | 1/3344 |
| 2026-06-24 09:06:50 | `log/baseline_eee42dfe63eb8d4e2d7fe894d843f55149e6ef1a.log` | 1/3341 |
| 2026-06-24 07:26:28 | `log/baseline_1f1d282bc0f2b4f113004c6db051dee0985a45f1.log` | 1/3327 |
| 2026-06-24 06:09:23 | `log/baseline_f754b3daf03422125c4a75f7d7523f9cc77810c0.log` | 1/3319 |
| 2026-06-24 05:07:13 | `log/baseline_3cecf22a5d5e9198f4f2cf116d538f3ce33a9873.log` | 1/3317 |
| 2026-06-23 19:46:29 | `log/baseline_971dc733ee473db49c9e9da01ed826b471f7e359.log` | 2/3316 |
| 2026-06-23 15:42:42 | `log/baseline_6411e3cde7cfa80d8a547d652f65c2239fa392e7.log` | 1/3313 |
| 2026-06-23 13:56:41 | `log/baseline_a2bb20a9f6d1193581c77263bb30493e1feccd7b.log` | 1/3304 |
| 2026-06-23 12:37:08 | `log/baseline_4964159f4c996263ce07c83a3ced9564c9d0b0c0.log` | 1/3292 |
| 2026-06-23 11:05:56 | `log/baseline_552929f567146fefbd6f7e6ea9fc0f93ccc53578.log` | 1/3280 |
| 2026-06-23 09:38:51 | `log/baseline_c159a130e44bb409fa53141281ed881ad507c3cd.log` | 1/3266 |
| 2026-06-23 08:18:06 | `log/baseline_adac41f8e81c76714ffcdfcc26c6e215b97e4757.log` | 1/3252 |
| 2026-06-22 21:12:55 | `log/baseline_d40a399087909301d586916c6e377066b4ad6423.log` | 1/3242 |
| 2026-06-22 18:32:57 | `log/baseline_ffb6f49151ebeeb337cd350c38acac3a334bda06.log` | 1/3239 |
| 2026-06-22 16:58:53 | `log/baseline_52ed4f2823dbcee3c3be62c54d075e86890cbb25.log` | 1/3238 |
| 2026-06-22 09:21:51 | `log/baseline_1ac2f53d961432f92886d46a92831eff3c4d6650.log` | 2/3237 |
| 2026-06-17 20:52:55 | `log/baseline_7aad1d838ddbeae5429911250eff0697282d406b.log` | 1/3234 |
| 2026-06-17 20:32:50 | `log/baseline_dcd5b72e864b05919ff0801fdee14dfb1df8114e.log` | 1/3231 |
| 2026-06-17 10:41:50 | `log/baseline_c21ba6f7d809a9c098785b99ebac2d70137d8984.log` | 1/3231 |
| 2026-06-17 02:34:56 | `log/baseline_690558bec3669ade0afeca48a602c7f99af3ceb4.log` | 1/3228 |
| 2026-06-16 21:07:03 | `log/baseline_807f2ef9e63235592e19131ba900c1935370209a.log` | 1/3222 |
| 2026-06-16 18:08:28 | `log/baseline_73543a60537a69f56db3502cef9362eb6edec870.log` | 0/3208 |


## Current Failed Rows

Source:
`log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`,
2026-07-10 04:20:43, `99% tests passed, 34 tests failed out of 3397`.

### Backend Rows

| Test ID | Row |
| ---: | --- |
| 92 | `backend_dump_riscv64_stack_passed_parameter_home_publication` |
| 103 | `backend_dump_riscv64_scalar_compare_frame_slot_destination` |
| 109 | `backend_dump_riscv64_prepared_fused_compare_call_result_predicate` |
| 120 | `backend_codegen_route_riscv64_loop_carried_pointer_postincrement` |
| 122 | `backend_codegen_route_riscv64_duff_fallthrough_pointer_update_producers` |
| 127 | `backend_codegen_route_riscv64_i16_local_array_select_store` |
| 150 | `backend_dump_riscv64_byval_aggregate_fixed_call` |
| 151 | `backend_codegen_route_riscv64_byval_aggregate_fixed_call` |
| 154 | `backend_dump_riscv64_byval_preserved_pointer_args` |
| 155 | `backend_codegen_route_riscv64_byval_preserved_pointer_args` |
| 165 | `backend_codegen_route_riscv64_byval_formal_gpr_publication` |
| 172 | `backend_dump_riscv64_function_pointer_return_chain` |
| 183 | `backend_obj_runtime_rv64_prepared_object_data_static_local_storage` |
| 184 | `backend_obj_runtime_rv64_prepared_object_data_static_local_initialized_storage` |
| 192 | `backend_rv64_runtime_riscv64_loop_carried_pointer_postincrement` |
| 193 | `backend_rv64_runtime_riscv64_duff_fallthrough_pointer_update_producers` |
| 195 | `backend_rv64_runtime_riscv64_i16_local_array_select_store` |
| 207 | `backend_rv64_runtime_riscv64_byval_aggregate_fixed_call` |
| 208 | `backend_rv64_runtime_riscv64_byval_preserved_pointer_args` |
| 209 | `backend_rv64_runtime_riscv64_byval_formal_gpr_publication` |
| 210 | `backend_obj_runtime_rv64_frame_slot_pointer_arg_preserves_payload` |
| 219 | `backend_obj_runtime_rv64_callee_saved_gpr_live_across_call` |
| 236 | `backend_rv64_runtime_packed_local_member_offsets` |
| 256 | `backend_riscv_object_emission` |
| 284 | `backend_aarch64_instruction_dispatch` |
| 297 | `backend_prepare_liveness` |
| 299 | `backend_prepare_frame_stack_call_contract` |
| 300 | `backend_prepared_printer` |
| 301 | `backend_prealloc_inline_asm` |
| 314 | `backend_cli_dump_prepared_bir_exposes_contract_sections` |
| 318 | `backend_cli_dump_prepared_bir_local_arg_call_contract` |
| 322 | `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication` |

### LLVM Torture Rows

| Test ID | Row |
| ---: | --- |
| 1941 | `llvm_gcc_c_torture_src_20040709_2_c` |
| 1942 | `llvm_gcc_c_torture_src_20040709_3_c` |

## Failure-Count Change Points

This table records every observed failure-count transition in
`log/baseline_*.log`, sorted oldest to newest. The newest high-signal current
transition is the clean `0/3376` log at 2026-07-09 12:47 followed by `13`,
`20`, `31`, `32`, and the current `34` plateau.

| Timestamp | Path | Change | Failed/Total |
| --- | --- | --- | ---: |
| 2026-06-16 18:08:28 | `log/baseline_73543a60537a69f56db3502cef9362eb6edec870.log` | initial | 0/3208 |
| 2026-06-16 21:07:03 | `log/baseline_807f2ef9e63235592e19131ba900c1935370209a.log` | 0 -> 1 | 1/3222 |
| 2026-06-22 09:21:51 | `log/baseline_1ac2f53d961432f92886d46a92831eff3c4d6650.log` | 1 -> 2 | 2/3237 |
| 2026-06-22 16:58:53 | `log/baseline_52ed4f2823dbcee3c3be62c54d075e86890cbb25.log` | 2 -> 1 | 1/3238 |
| 2026-06-23 19:46:29 | `log/baseline_971dc733ee473db49c9e9da01ed826b471f7e359.log` | 1 -> 2 | 2/3316 |
| 2026-06-24 05:07:13 | `log/baseline_3cecf22a5d5e9198f4f2cf116d538f3ce33a9873.log` | 2 -> 1 | 1/3317 |
| 2026-06-24 17:26:11 | `log/baseline_cad43111e89f360fbf31222de46c944aabd25e04.log` | 1 -> 0 | 0/3345 |
| 2026-06-25 08:54:20 | `log/baseline_84503bad13d02c40d12adae011165374bd1839be.log` | 0 -> 1 | 1/3351 |
| 2026-06-25 11:59:58 | `log/baseline_b1889cc95b329a12de0ca570abad2895e35f6db4.log` | 1 -> 0 | 0/3352 |
| 2026-06-25 16:31:23 | `log/baseline_9553a9c68fd14df260ca1b86ec465eb79e254a36.log` | 0 -> 2 | 2/3352 |
| 2026-06-25 18:56:30 | `log/baseline_8278eed37ed3ba1ce66eab135108492c44b03d8f.log` | 2 -> 1 | 1/3353 |
| 2026-06-25 21:43:46 | `log/baseline_ac43eeb74d788ae59eed3c0fe387dd1569c08845.log` | 1 -> 0 | 0/3353 |
| 2026-06-26 04:55:08 | `log/baseline_1bc45813aeb02e5d133950045ca01c31282adc57.log` | 0 -> 2 | 2/3353 |
| 2026-06-26 06:46:52 | `log/baseline_ab05dc368e846e54cf6b01282f564c1ec91fd527.log` | 2 -> 9 | 9/3353 |
| 2026-06-26 09:00:03 | `log/baseline_323b42720a2f13998b61b02da71fb4b6876d5688.log` | 9 -> 0 | 0/3355 |
| 2026-06-28 22:23:27 | `log/baseline_22bbe64311e13f55fafd54affcd821de5eedaaa7.log` | 0 -> 1 | 1/3369 |
| 2026-06-30 03:52:56 | `log/baseline_8a07e2d14eb321aab2a1be757a2729a3e7b933bb.log` | 1 -> 0 | 0/3356 |
| 2026-06-30 10:09:05 | `log/baseline_64ff6ae71dba8c58d954f01b4252e23de2153b64.log` | 0 -> 1 | 1/3356 |
| 2026-07-01 06:41:50 | `log/baseline_9680a9509bab8b7a73ec474f6b873caa3a800c18.log` | 1 -> 3 | 3/3357 |
| 2026-07-01 08:30:01 | `log/baseline_c59a4b57e79c5da6a766890a9c31448eb2cdc293.log` | 3 -> 1 | 1/3357 |
| 2026-07-01 18:39:41 | `log/baseline_772117092529d6e3a2c650e7a6463d4679331220.log` | 1 -> 2 | 2/3359 |
| 2026-07-01 20:21:22 | `log/baseline_d6feec796871daa7076d02744c4e9aa4a4347505.log` | 2 -> 1 | 1/3361 |
| 2026-07-02 15:39:02 | `log/baseline_f65d7d587753a000b6d4c1b23390e5931f1ff293.log` | 1 -> 0 | 0/3374 |
| 2026-07-02 21:40:48 | `log/baseline_61735dfafdf1321e5b8dc2b4a241229de32fcfd9.log` | 0 -> 1 | 1/3374 |
| 2026-07-08 04:30:43 | `log/baseline_6801046c7ec1a80899fc5961fe9a828850cc7e80.log` | 1 -> 0 | 0/3375 |
| 2026-07-08 17:30:12 | `log/baseline_ee3b1c5559bb0d9589be0e59b637eba9f32312d2.log` | 0 -> 5 | 5/3375 |
| 2026-07-08 19:11:17 | `log/baseline_cd470040edb572b63ee74b7e1c42dbecdcd2ed85.log` | 5 -> 0 | 0/3375 |
| 2026-07-09 15:56:55 | `log/baseline_9ae9ddfce0163d04b7df19e956175a6c8ccb1083.log` | 0 -> 13 | 13/3376 |
| 2026-07-09 17:57:21 | `log/baseline_8a1ed1685aaa22a36153bc13c7acf08aeda05cd1.log` | 13 -> 20 | 20/3376 |
| 2026-07-09 19:35:50 | `log/baseline_11ee2da0f73d9328066ae05b84d75028df1adce7.log` | 20 -> 31 | 31/3376 |
| 2026-07-09 22:53:39 | `log/baseline_24c42f8da1ba7099134e97af222391de11ec5753.log` | 31 -> 32 | 32/3389 |
| 2026-07-10 00:58:16 | `log/baseline_575142db412ec5e846e77a6abcef8a77d7e97d83.log` | 32 -> 34 | 34/3394 |

## 657 Representative Reconciliation

The 657 summaries conflict only if read without timestamps:

- Older evidence, 2026-07-10 04:12:24:
  `build/agent_state/657_step4_representative_proof/summary.md` says the
  representative `loop-2e.c` runtime still mismatched clang and that the object
  wrote both the local cursor update and caller-visible stored pointer back to
  the local `%lv.param.q` home at `0(sp)`.
- Newer evidence, 2026-07-10 04:18:53:
  `build/agent_state/657_step3_representative_pointer_value_store/summary.md`
  says the focused contracts passed, the representative object now emits
  `sd t1,0(sp)` for the local cursor writeback and `sd t1,0(s1)` for
  `base=pointer_value stored=%t8 pointer=%t9`, and the representative
  `loop-2e.c` RV64 object-runtime comparison now passes against clang.

Conclusion: for Step 2 and later classification, the authoritative 657
representative state is the newer 04:18 pass. The older 04:12 mismatch remains
useful historical evidence for the repaired owner, but it should not be treated
as a current representative failure.

## Step 1 Completion Check

- Newest broad baseline authority is
  `log/baseline_f3bf820c180dd4638ebd4db37e1223b759103665.log`.
- Newest representative 657 authority is
  `build/agent_state/657_step3_representative_pointer_value_store/summary.md`.
- Current failed backend and LLVM torture rows are recorded above.
- Failure-count change points are recorded above, with the current triage
  expansion from `0` to `34` failures called out.
