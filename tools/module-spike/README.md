# RM-2 — Named module / QObject / QML doğrulaması

Üretim hedeflerinden bağımsız teknik deney. Root CMake'e eklenmez.
`runmark.spike.domain` C++23 named module'ünü yalnız `bridge.cpp` import eder;
MOC, module tiplerini görmeyen normal `bridge.h` üzerinde çalışır.
Test, QML'den `Q_INVOKABLE` çağrısının module sonucunu `Q_PROPERTY` ve
`NOTIFY` üzerinden QML binding'ine taşıdığını denetler. GUI gerektirmez.
`import std`, header units, QObject-in-cppm ve QML paket dağıtımı test edilmez.

## Çalıştırma

Repository kökünden, boş bir build diziniyle:

```sh
cmake -S tools/module-spike -B build/module-spike -G Ninja \
  -DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++ \
  -DCMAKE_PREFIX_PATH=/Users/techalp/Qt/6.11.1/macos \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build/module-spike --parallel 4
ctest --test-dir build/module-spike --output-on-failure
cmake --build build/module-spike --parallel 4
```

Linux'ta compiler ve Qt yollarını kurulu LLVM/Qt'ye göre değiştir.
Windows'ta MSVC Developer PowerShell ve Ninja kullan; compiler seçimini
`-DCMAKE_CXX_COMPILER=cl` yap, Qt yolunu MSVC kitine yönelt.
Bunlar henüz doğrulanmış Linux/Windows baseline'ları değildir.

Incremental kontrol: `domain.cppm` içindeki `value + 1` ifadesini geçici
olarak `value + 2` yap. Build importer `bridge.cpp` dosyasını da derlemeli;
CTest başarısız olmalı. `+1` geri alındığında build ve test tekrar geçmeli.
Sonunda değişikliksiz build `ninja: no work to do.` vermeli.

## Ölçüm — 2026-09-22

| Platform | Toolchain | Clean / QML | Incremental |
|---|---|---|---|
| macOS arm64 | LLVM Clang 23.1.1, CMake 4.4.3, Ninja 1.13.2, Qt 6.11.1 | PASS, CTest 1/1 | PASS; +2 beklenen FAIL, +1 PASS; no-op PASS |
| Linux | Yerel `scs-dev-arm64:latest`: Qt 6.11.1, CMake 4.4.3, Ninja 1.11.1; GCC 13.3, Clang yok | Uygun module compiler bekliyor | Bekliyor |
| Windows | Henüz ölçülmedi | Bekliyor | Bekliyor |

Runmark execution: `20260922T103319Z-RM-2`. macOS sonucu minimum compiler
sürümü iddiası değildir. Üç OS ölçümü olmadan RM-2 kapanmaz ve RM-3 başlamaz.
Üretim C++20 baseline'ı değiştirilmedi.

Referans: [CMake C++ Modules](https://cmake.org/cmake/help/latest/manual/cmake-cxxmodules.7.html).
