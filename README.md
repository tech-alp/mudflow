# Mudflow

AI ile geliştirilen projelerde plan, kod, Git state ve agent context'in
birbirinden kopmasını engelleyen local-first execution control plane.

Doküman: [docs/](docs/README.md)

## Build

Gereken: CMake 3.21+, Qt 6.11+, C++20 derleyici.

```sh
export QT6_ROOT=~/Qt/6.11.1/macos      # kendi Qt yolun
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

`QT6_ROOT` ayarlı değilse CMake Qt'yi sistem yollarında arar.
Kalıcı bir yapılandırma istersen `CMakeUserPresets.json` yaz (git ignore'da).

## Kullanım

```sh
mudflow inspect                                   # project.json'u doğrula
mudflow status                                    # findings üret
mudflow start MF-1 --agent codex                  # verified worktree aç
mudflow evidence <exec> --kind test --summary "…" # kanıt kaydet
mudflow note <exec> --kind unresolved --text "…"  # açık madde kaydet
mudflow finish <exec> --outcome finished          # ledger + handoff kapat
mudflow resume [<task|exec>] --markdown           # bir sonraki oturumun bağlamı
```

Seçici verilmezse en son execution seçilir.

Durum `.mudflow/` altında tutulur: `ledger/`, `evidence/`, `handoffs/`.
Şema: [docs/DATA_MODEL.md](docs/DATA_MODEL.md)

## Ajan eklentisi

`resume`'u oturum başında otomatik çağıran SessionStart hook'u
[plugins/mudflow-agent](plugins/mudflow-agent) altındadır. Hook CLI'yi
`PATH`'te arar, o yüzden önce kur:

```sh
cmake --install build --prefix ~/.local    # ~/.local/bin PATH'te olmalı
claude plugin marketplace add .
claude plugin install mudflow-agent@mudflow
```

Projenin `project.json`'una `"hooks_expected": true` yazıldığında hook hiç
çalışmamışsa `status` bunu `context.hooks_not_observed` olarak bildirir.
