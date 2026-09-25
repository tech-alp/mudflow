# Konumlanma notu

Tarih: 2026-09-25
Durum: Değerlendirme. Ürün kararı değil; [yakalama günlüğü](#değeri-nasıl-ölçeriz) sonuç verene kadar hipotez.

## Tek cümle

Runmark ajan çalıştırmaz, pano tutmaz, oturum kaydetmez; başkalarının ürettiği kaydı **okur** ve "ajanın dediği" ile "gerçekte olan"ı ayrı tutup plana bağlı, açıklanabilir bir **"devam etmek güvenli mi?"** kararı verir.

## Hakem, stadyum değil

Bir maçta üç ayrı iş var:

- **Stadyum:** oyuncuları sahaya çıkarır, maçı oynatır. → Ajanı başlatan, worktree açan, görevi dağıtan araçlar (Multica, Conductor, CAO, Claude Squad).
- **Kamera:** her şeyi kaydeder. → Oturum dökümünü saklayan araçlar (Entire, Claude/Codex'in kendi transcript dosyaları).
- **Hakem:** kayda bakıp "bu gol sayılır mı?" der. Oyuncunun "gol attım" demesi yetmez.

Bugün stadyum ve kamera kalabalık; hakem boş. Runmark'ın yeri hakemlik. Stadyum veya kamera yazmaya başlarsak kalabalık bir alanda bir kopya daha oluruz; üstelik Anthropic ve OpenAI bu özellikleri kendi araçlarına ekledikçe ince katmanlar erir.

Somut örnek: bu makinede Multica kurulu. Multica ajanı başlatır, bir çalışma dizininde çalıştırır ve "run completed" der. Runmark o dizinin Git durumunu, ajanın transcript'ini ve planı okuyup şunu söyler: "completed dendi ama son `ctest` exit 1 ile bitti" ya da "base 14 commit geride". Multica'yı değiştirmez, onunla yarışmaz; **girdisi** yapar.

Hakemin iki yapısal avantajı var:

1. **Tarafsızlık.** Claude kendi ödevini, Codex kendi ödevini notlamamalı. Hiçbirinin satıcısı olmayan bir araç ikisini aynı kuralla okur.
2. **Kanıt sırası.** ajanın sözü < runtime kaydı (transcript) < Runmark'ın kendi ölçtüğü (Git). Her bulgu hangi basamaktan geldiğini söyler ([ADR-002](DECISIONS.md)).

## Rakip haritası (2026-09)

| Grup | Örnekler | İlişki |
| --- | --- | --- |
| Ajan yapma framework'leri | LangChain, LangGraph, LangSmith | Rakip değil. Kendi ajanını yazanlar içindir; biz hazır ajanları denetleriz. |
| Paralel ajan çalıştırıcılar (stadyum) | Conductor, Claude Squad, CAO, Multica, Vibe Kanban | Yarışmıyoruz. `rmk start` worktree kısmı burayla çakışır; minimumda tutulur. |
| Oturum kaydı (kamera) | Entire (Checkpoints, MIT CLI) | En yakın komşu. Kaydeder, gördüğümüz kadarıyla yargılamaz. Risk: üstüne kural motoru eklemesi kısa adım. Fırsat: checkpoint'leri girdi olarak okumak. |
| Takım bilgisi dağıtımı | TeamAI (Tencent) | Komşu. Transcript'ten "ajana sormadan" sinyal çıkarma tekniği bizimle aynı yönde. |

## Ne yapmıyoruz (şimdilik)

- Ajan başlatma ve orkestrasyon ([ADR-020](DECISIONS.md)): yakalama günlüğü değer gösterene kadar ertelendi.
- Desktop'u ana ürün yapmak: desktop bir görünüm; ürün CLI + hook + bulgular.
- Plugin runtime.

## Değeri nasıl ölçeriz

**Yakalama günlüğü:** gerçek görevlerde Runmark'ın yakaladığı, ajanın ise kaçırdığı veya yanlış iddia ettiği her durum bir satır:

- yanlış "bitti" (son test kırmızı, kanıt yok),
- eski base üzerinde çalışma,
- kaybolmak üzere olan commit'siz iş,
- devirde kaybolan açık madde.

Karar kuralı: Runmark ve olympos üzerinde 20 gerçek görevde yakalama sıfıra yakınsa değer yoktur; bunu erken öğrenmek iyidir.

## Sıradaki adım

Transcript kanıtı: `rmk finish` ajanın Claude/Codex transcript'ini okur; test komutlarını ve çıkış kodlarını `source: runtime` kanıtı olarak yazar. Ajanın `rmk evidence` ile verdiği kanıt `source: agent` olarak kalır ve tek başına test kanıtı sayılmaz. Ayrıntı: [DATA_MODEL](DATA_MODEL.md).
