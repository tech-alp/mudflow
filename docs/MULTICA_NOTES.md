# Multica inceleme notu

Tarih: 2026-09-25  
Durum: Doküman ve kaynak kodu incelemesi. Gerçek görev pilotu yapılmadı; ürün kararı verilmedi.

## Çıkarım

Multica, Runmark için düşündüğümüz **agent çalıştırma, görev panosu ve oturum devamlılığı** alanlarının önemli bölümünü zaten sunuyor. Runmark'ın ayrı bir ürün olarak sınanması gereken iddiası, mevcut plan ve Git durumunu agent beyanından bağımsız okuyup açıklanabilir bir **“devam etmek güvenli mi?”** değerlendirmesi üretmesi. Bu farkı varsaymak yerine aynı gerçek görev üzerinde ölçmeliyiz. Runmark'ın hedefi [PRD](PRD.md) ve [Trust modeli](TRUST_MODEL.md) içinde tanımlı.

## Multica'da doğruladıklarımız

- **Akış:** Issue ataması run oluşturur; bağlı makinedeki daemon run'ı alır, yerel agent CLI'ını çalıştırır ve sonucu issue'ya yazar. Daemon ve backend Go ile yazılmıştır. [Çalışma akışı](https://multica.ai/docs/how-multica-works), [mimari](https://github.com/multica-ai/multica#architecture), [daemon kaynak kodu](https://github.com/multica-ai/multica/blob/main/server/internal/daemon/daemon.go), [run claim/sonuç API istemcisi](https://github.com/multica-ai/multica/blob/main/server/internal/daemon/client.go).
- **Daemon'ın gerekçesi:** Sunucu görevleri ve kayıtları koordine eder; daemon, kodun ve Claude Code/Codex kimlik bilgilerinin bulunduğu makinede agent'ı başlatır, çalışma dizinini yönetir ve sonucu geri bildirir. Bu işin arka planda, kuyruk/bağlantı kopması boyunca sürmesi için ayrı süreç kullanılır. [Daemon ve runtimes](https://multica.ai/docs/daemon-runtimes).
- **İş ve çalıştırma ayrımı:** Bir issue birden fazla run içerebilir; önceki run kayıtları korunur. Run'ın `completed` olması issue'nun `done` olması demek değildir. `done` genelde insan onayıyla veya ayarlanmış PR merge kuralıyla gelir. [Runs](https://multica.ai/docs/tasks), [Issues](https://multica.ai/docs/issues).
- **Pano ve proje:** Issue durumlarıyla board/list görünümü, proje kaynakları, bir projeye birden fazla repo veya yerel dizin bağlama ve PR/CI görünümü var. Bu, Runmark için ayrı Kanban yapma gerekçesini zayıflatıyor. [Issues](https://multica.ai/docs/issues), [Project resources](https://multica.ai/docs/project-resources), [GitHub integration](https://multica.ai/docs/github-integration).
- **Veri sınırı:** Kod dizini ve CLI kimlik bilgileri çalışan makinede kalır; sunucu issue, yorum, agent ayarı, run bağlamı/kayıtları/sonuçlarını tutar. Agent yanıtları kod parçaları içerebilir; özel ortam değişkenleri ve MCP yapılandırması sunucuda saklanır. Self-host kurulum web, API ve PostgreSQL'i; çalıştıran makine daemon ve agent CLI'larını gerektirir. [Veri sınırı](https://multica.ai/docs/daemon-runtimes), [self-host](https://multica.ai/docs/self-host-quickstart).
- **Güvenlik:** Agent varsayılan olarak daemon'ı çalıştıran işletim sistemi kullanıcısının yetkileriyle, pratikte sandbox olmadan çalışır. Multica ayrı Unix kullanıcısı, container veya VM önerir. [Security model](https://multica.ai/docs/security-model).
- **Lisans:** Kaynak, yalnız Apache-2.0 değildir; ek Multica License koşulları taşır. Üçüncü kişilere hosted servis veya ticari ürüne gömme için ticari lisans şartı; arayüz markası ve arayüzsüz kullanımda atıf koşulları var. Paketli ürünü denemek ile daemon/kodunu Runmark'a almak farklı kararlardır. Kod yeniden kullanımı öncesi lisans incelemesi gerekir. [LICENSE](https://github.com/multica-ai/multica/blob/main/LICENSE).

## Runmark ile sınır

| Alan | Multica | Runmark bugün / hedef |
| --- | --- | --- |
| Agent çalıştırma | Daemon yerel CLI'ı başlatır; kuyruk, run kaydı ve sonuç bildirimi var. | `rmk start` worktree ve ledger açar, agent sürecini başlatmaz. Claude/Codex launch [MVP](MVP.md) hedefidir. |
| Görev/pano | Issue, durum, board, atama, yorum ve PR bağı var. | Kompleks Kanban [MVP](MVP.md) dışında. |
| Handoff | Issue geçmişi ve birden fazla run saklanır; retry uygun durumda önceki dizin/oturumu kullanır. | `rmk finish` Git değişimini ve handoff'u yazar; `rmk resume` bağlam ve bulguları üretir. Farklı agent'a gerçek devir henüz karşılaştırılmadı. |
| Doğrulama | Çalıştırma kaydı, PR ve CI bilgisi gösterir. | Plan ↔ evidence, remote base, worktree ve context için açıklanabilir bulgular hedeflenir. Multica'nın aynı kontrolleri sağlayıp sağlamadığı pilot sorusudur. |
| Veri ve işletim | Sunucu + PostgreSQL + yerel daemon; self-host seçeneği var. | Yerel CLI ve `.runmark/` kayıtları. Bu, kurulum/veri kontrolünde olası avantaj; kullanım değeri ölçülmeli. |

Runmark'ın bugünkü `start`/`finish` davranışı [workflow.cpp](../libs/application/src/workflow.cpp) içinde; roadmap maddeleri tamamlanmış özellik sayılmamalı.

## Gerçek görev pilotu: karar ölçütleri

1. Runmark'tan küçük, gerçek bir görev seç; kabul ölçütü, repo ve plan bağlantısını issue'ya koy. Claude Code çalışsın, sonra aynı işi Codex devralsın. Elle oturum dökümü taşımadan doğru değişiklikleri, kararları ve kalan işi bulabiliyor mu?
2. Her geçişte branch/worktree, başlangıç base SHA'sı, değişen dosyalar, commit'ler, çalıştırılan testler ve hata/engel kaydı görülebiliyor mu? Orijinal checkout korunuyor mu?
3. Bir run'ı yarıda kesip tekrar dene. Dosya/oturum devamlılığı, hatanın görünürlüğü ve `run completed` ile `issue done` ayrımı beklenen gibi mi?
4. Runmark'ın `status`/`resume` bulgularıyla karşılaştır: eski base, plan değişimi, plansız execution, test kanıtı eksikliği gibi hangi sorulara Multica doğrudan cevap veriyor? Hangisi yalnız agent beyanı veya issue durumuna dayanıyor?
5. Kurulum, sunucu işletimi, veri/secret sınırı ve lisans koşullarını kabul edilebilir buluyor muyuz?

**Karar kuralı:** Pilot agent koordinasyonu ve pano ihtiyacını karşılıyorsa bunları yeniden yazma. Ölçülebilir plan/Git/evidence doğrulama açığı kalırsa Runmark'ı bu sınırda, mümkünse Multica'dan bağımsız araç olarak geliştir. Daemon kodunu çatallama veya ürüne gömme kararını ayrı teknik ve lisans değerlendirmesine bırak. Runmark/Nexroz için ortak Markdown modülü ayrı mimari konudur; bu pilot onun tasarımına karar vermez.
