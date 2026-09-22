# Runmark POC Plan

Tarihsel POC kaydıdır. Ürün artık Runmark olarak adlandırılır; `MF-*` task
kimlikleri ve aşağıdaki mevcut `rmk` komutu evidence bağlantılarını
korumak için değiştirilmedi. Hedef adlandırma [Architecture](../ARCHITECTURE.md)'dadır.

Görevler `- [ ]` / `- [x]` checklist maddesi olarak yazılır ve task ID taşır.
Format `project.json` içindeki `task_id_pattern` ile eşleşmezse `rmk status`
`plan.no_parsable_tasks` uyarısı verir — kurallar sessizce kör kalmaz.

- [x] MF-1 — Core POC

  `project.json`, Git health, execution ledger, evidence ve handoff akışını
  doğrula.

- [ ] MF-2 — Plan-linked execution doğrulaması

  Bir execution'ı plan task'ına bağla; commit, test evidence ve handoff üret.

- [x] MF-3 — GitHub base üzerinde plan-linked execution

  GitHub `origin/main` üzerinden worktree aç; commit, test evidence ve handoff
  ile plan bağlantısını doğrula.
