#!/usr/bin/env python3
"""Generates docs/assets/runmark-architecture.svg and runmark-execution-loop.svg."""
import sys
from xml.sax.saxutils import escape

INK, MUTED, DEBT = "#1f2328", "#57606a", "#cf222e"
C = {
    "surface": ("#f4f0fc", "#5b3fa6"), "present": ("#fbf1f7", "#8e3a78"),
    "app": ("#f6f8fa", "#57606a"), "domain": ("#eef4fd", "#1f5fbf"),
    "infra": ("#f3f7fd", "#1f5fbf"), "store": ("#fdf5e8", "#b36b00"),
    "agent": ("#eef8f0", "#2d7a3e"), "grey": ("#f6f8fa", "#57606a"),
    "debt": ("#fff5f5", DEBT),
}
FONT = "-apple-system, 'Segoe UI', Helvetica, Arial, sans-serif"


class Svg:
    def __init__(self, w, h, title):
        self.w, self.h, self.out = w, h, []
        self.text(w / 2, 50, title, 34, INK, "middle", True)

    def text(self, x, y, s, size=15, fill=INK, anchor="start", bold=False):
        weight = ' font-weight="bold"' if bold else ""
        self.out.append(f'<text x="{x}" y="{y}" font-size="{size}" fill="{fill}"{weight} text-anchor="{anchor}">{escape(s)}</text>')

    def rect(self, x, y, w, h, fill, stroke, sw=2, dashed=False, rx=8):
        dash = ' stroke-dasharray="7 5"' if dashed else ""
        self.out.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{rx}" fill="{fill}" stroke="{stroke}" stroke-width="{sw}"{dash}/>')

    def panel(self, x, y, w, h, key, title, sub=None, size=22):
        fill, stroke = C[key]
        self.rect(x, y, w, h, fill, stroke)
        self.text(x + 18, y + 32, title, size, stroke, bold=True)
        if sub:
            self.text(x + 18, y + 54, sub, 13, MUTED)

    def box(self, x, y, w, h, key, title, sub=None, dashed=False, debt=None):
        stroke = C[key][1]
        self.rect(x, y, w, h, "#fbfbfb" if dashed else "#ffffff", stroke, 1.5, dashed, 5)
        cx = x + w / 2
        if sub:
            self.text(cx, y + h / 2 - 2, title, 15, INK, "middle")
            self.text(cx, y + h / 2 + 15, sub, 11, MUTED, "middle")
        else:
            self.text(cx, y + h / 2 + 5, title, 15, INK, "middle")
        if debt:
            self.badge(x + w - 4, y + 4, debt)

    def badge(self, x, y, label):
        self.out.append(f'<circle cx="{x}" cy="{y}" r="13" fill="{DEBT}"/>')
        self.text(x, y + 4.5, label, 12, "#ffffff", "middle", True)

    def row(self, x, y, w, h, key, items):
        gap = 14
        bw = (w - gap * (len(items) - 1)) / len(items)
        for i, item in enumerate(items):
            title, sub, *rest = item + (None,) * (4 - len(item))
            dashed, debt = rest[0] or False, rest[1]
            self.box(x + i * (bw + gap), y, bw, h, key, title, sub, dashed, debt)

    def arrow(self, d, color=INK, dashed=False, label=None, lx=0, ly=0, anchor="start"):
        dash = ' stroke-dasharray="6 5"' if dashed else ""
        self.out.append(f'<path d="{d}" fill="none" stroke="{color}" stroke-width="1.8"{dash} marker-end="url(#a)"/>')
        if label:
            self.text(lx, ly, label, 12, color, anchor)

    def line(self, x1, y1, x2, y2, color, dashed=True, sw=2.5):
        dash = ' stroke-dasharray="10 6"' if dashed else ""
        self.out.append(f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" stroke="{color}" stroke-width="{sw}"{dash}/>')

    def render(self):
        head = (f'<svg xmlns="http://www.w3.org/2000/svg" width="{self.w}" height="{self.h}" '
                f'viewBox="0 0 {self.w} {self.h}" font-family="{FONT}">\n'
                '<defs><marker id="a" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="8" markerHeight="8" '
                'orient="auto-start-reverse"><path d="M0,0 L10,5 L0,10 z" fill="context-stroke"/></marker></defs>\n'
                f'<rect width="{self.w}" height="{self.h}" fill="#ffffff"/>')
        return "\n".join([head, *self.out, "</svg>\n"])


def architecture():
    s = Svg(1700, 1240, "Runmark Architecture")
    L, W, BX = 30, 1250, 250          # left column, its width, first box x
    BW = L + W - 16 - BX              # box row width

    # Surfaces: who looks at the result.
    s.panel(L, 90, W, 110, "surface", "Yüzeyler", "sonucu kim görür")
    s.row(BX, 114, BW, 62, "surface", [
        ("rmk CLI", "JSON · exit 0/1/2 (TC-007)"),
        ("Desktop · macOS", "Qt Quick + Merce · salt okunur"),
        ("Ajan oturumu", "resume paketi bağlama girer"),
        ("Linux · Windows", "hedef", True)])

    # Presentation: in-process formatting only.
    s.panel(L, 225, W, 110, "present", "Sunum", "süreç içi · biçime çevirir")
    s.row(BX, 249, BW, 62, "present", [
        ("json.cpp · toJson", "apps/cli · tek JSON yeri"),
        ("resumeMarkdown", "apps/cli'de kilitli", False, "B4"),
        ("libs/ui-shell", "ViewModel · FindingModel"),
        ("Plugin runtime · QML host", "hedef (ADR-018)", True)])

    s.panel(L, 360, W, 100, "app", "Application", "use case · tipli sonuç (TC-012)")
    s.row(BX, 379, BW, 62, "app", [
        ("inspect · status", "observe → evaluate", False, "B2"),
        ("start · finish", "worktree · preserve · handoff"),
        ("resume", "paket · last activity"),
        ("evidence · note", "ledger'a ekler")])

    # Infrastructure and Domain sit side by side: the application calls both,
    # infrastructure produces Facts, domain only ever sees Facts.
    HW = (W - 20) / 2
    IX, DX, TOP = L, L + HW + 20, 510
    s.panel(IX, TOP, HW, 250, "infra", "Infrastructure", "dış dünyayı okur → Facts")
    s.panel(DX, TOP, HW, 250, "domain", "Domain", "C++23 module · saf · bağımlılığı yok")
    gw, gh = (HW - 50) / 2, 70
    for i, item in enumerate([("git", "fetch · merge-base · preserve"), ("ledger", "JSONL · kilitsiz"),
                              ("handoff · plan", "disk I/O · tam-token ID"), ("config_io · paths", ".runmark yerleşimi")]):
        s.box(IX + 18 + (i % 2) * (gw + 14), TOP + 72 + (i // 2) * (gh + 16), gw, gh, "infra", *item)
    for i, item in enumerate([("Facts", "ölçülen, yorumsuz · tipli olaylar"), ("rules · evaluate", "Facts → Finding", False, "B3"),
                              ("ProjectConfig::parse", "doğrulama"), ("domain_purity", "I/O · QObject yasak (CTest)")]):
        s.box(DX + 18 + (i % 2) * (gw + 14), TOP + 72 + (i // 2) * (gh + 16), gw, gh, "domain", *item)

    s.panel(L, 800, W, 140, "store", "Workspace & Persistence", "yerel, Git'in yanında")
    s.row(BX, 846, BW, 62, "store", [
        ("project.json", "commit'lenir"), ("ledger/*.jsonl", "execution başına"),
        ("handoffs/*.md", "ölçülen · iddia · açık"), ("refs/runmark/preserved/*", "commit'siz iş"),
        ("git worktrees", "task başına")])
    s.text(BX + BW / 2, 930, "evidence/ · hook-observed.json · .git/info/exclude (paylaşılan .gitignore'a dokunulmaz)", 12, MUTED, "middle")

    # Dependency arrows: presentation → application, application → infrastructure
    # and domain, infrastructure → domain (uses the Facts types), infrastructure → disk.
    s.arrow("M640,200 V225")
    s.arrow("M640,335 V360")
    s.arrow(f"M{IX + HW / 2},460 V{TOP}")
    s.arrow(f"M{DX + HW / 2},460 V{TOP}")
    s.arrow(f"M{IX + HW},{TOP + 125} H{DX}")
    s.arrow(f"M{IX + HW / 2},{TOP + 250} V800")

    # Agent clients: a separate process that calls rmk, not an in-process layer.
    s.panel(1320, 90, 350, 250, "agent", "Ajan istemcileri", "ayrı süreç · rmk'yi dışarıdan çağırır", 20)
    s.box(1338, 150, 314, 46, "agent", "Claude Code", "hook: düz stdout")
    s.box(1338, 204, 314, 46, "agent", "Codex", "hooks = dosya yolu · Trust")
    s.box(1338, 258, 314, 62, "agent", "runmark-agent plugin", "SessionStart hook · skill · iki runtime'da aynı")
    s.arrow("M1338,289 H1300 V75 H372 V114", "#2d7a3e",
            label="rmk resume --markdown --hook · start · evidence · note · finish", lx=760, ly=70, anchor="middle")

    s.panel(1320, 360, 350, 210, "agent", "Gerçeklik kaynakları", None, 20)
    s.box(1338, 402, 314, 46, "agent", "Git remote (origin)", "doğrulanmış base · ADR-007")
    s.box(1338, 456, 314, 46, "agent", "Plan · Markdown", "[x] beyan, kanıt değil")
    s.box(1338, 510, 314, 46, "agent", "Jira · GitHub Issues · MudIssue", "hedef", True)
    s.arrow(f"M1320,480 H1300 V780 H500 V{TOP + 250}", "#2d7a3e", True)

    s.panel(1320, 590, 350, 190, "store", "Tasarım & araç zinciri", None, 20)
    s.box(1338, 632, 314, 42, "store", "Merce v1.2.0", "FetchContent · statik · desktop profili")
    s.box(1338, 680, 314, 42, "store", "Qt 6.11 · LabsStyleKit")
    s.box(1338, 728, 314, 42, "store", "CMake 4.4 · Ninja · LLVM", "cforgo ölçüldü, alınmadı")

    s.panel(1320, 800, 350, 140, "grey", "Ertelendi · ADR-020", None, 20)
    s.box(1338, 840, 314, 42, "grey", "launch agent", "ön koşul", True)
    s.box(1338, 888, 314, 42, "grey", "assign · handoff · send_message", "ondan sonra", True)

    # Known debts, measured in the code on 2026-09-25.
    s.panel(L, 965, 830, 255, "debt", "Bilinen borçlar", "kodda doğrulandı · sıra = önerilen çözüm sırası", 20)
    debts = [
        ("B2", "Application somut infrastructure'a bağlı, port yok", "→ testler gerçek git deposu kurmak zorunda"),
        ("B3", "Finding ID'leri rules.cpp'de dağınık, katalog DATA_MODEL.md'de elle", "→ kodda tek kural kaydı"),
        ("B4", "resumeMarkdown apps/cli içinde", "→ desktop aynı paketi gösteremez"),
    ]
    for i, (tag, what, why) in enumerate(debts):
        y = 1040 + i * 46
        s.badge(L + 34, y - 5, tag)
        s.text(L + 58, y, what, 14)
        s.text(L + 58, y + 17, why, 12, MUTED)

    s.panel(880, 965, 400, 255, "grey", "Hedef", None, 20)
    for i, g in enumerate(["Plan, kod, Git ve ajan bağlamı kopmasın.",
                           "Kanıt > iddia (ADR-002).",
                           "Bilinmeyen ≠ temiz: sessiz yeşil yok.",
                           "CLI, desktop ve ajan aynı sonuçtan beslenir."]):
        s.text(900, 1040 + i * 32, "• " + g, 15)

    s.rect(1320, 965, 350, 255, "#ffffff", "#8c959f")
    s.text(1338, 997, "Gösterim", 20, INK, bold=True)
    s.arrow("M1338,1030 H1390")
    s.text(1402, 1035, "bağımlılık = çağrı yönü", 13)
    s.arrow("M1338,1066 H1390", "#2d7a3e")
    s.text(1402, 1071, "süreçler arası çağrı", 13)
    s.rect(1338, 1094, 52, 18, "#fbfbfb", MUTED, 1.2, True, 3)
    s.text(1402, 1108, "kesikli = hedef / ertelendi", 13)
    s.badge(1364, 1140, "B")
    s.text(1402, 1145, "bilinen borç", 13)
    s.text(1338, 1190, "Döngü ve güven sınırı:", 13, MUTED)
    s.text(1338, 1208, "runmark-execution-loop.svg", 13, MUTED)
    return s.render()


def loop():
    s = Svg(1700, 960, "Execution döngüsü ve güven sınırı")
    top, mid, bot = 90, 470, 790                    # band edges
    s.rect(30, top, 1640, mid - top, "#eef8f0", "#2d7a3e")
    s.rect(30, mid, 1640, bot - mid, "#fdf5e8", "#b36b00")
    s.text(50, top + 34, "Ölçülen — Runmark kendisi gözlemler", 22, "#2d7a3e", bold=True)
    s.text(50, top + 56, "git, dosya sistemi ve saat; ajanın sözüne dayanmaz", 13, MUTED)
    # The claim band's title sits in the empty fifth column so no arrow crosses it.
    s.text(1375, 625, "Beyan", 22, "#b36b00", bold=True)
    for i, ln in enumerate(["ajan veya insan söyler,", "Runmark yalnız kaydeder."]):
        s.text(1375, 652 + i * 20, ln, 15, "#b36b00")
    for i, ln in enumerate(["zayıf kanıt; resume paketinde", "ayrı başlıkta ve 'unverified'", "etiketiyle durur"]):
        s.text(1375, 704 + i * 17, ln, 13, MUTED)
    s.line(30, mid, 1670, mid, DEBT)
    s.text(1650, mid - 10, "güven sınırı (ADR-002)", 15, DEBT, "end", True)

    cols = [60, 385, 710, 1035, 1360]
    W = 290
    steps = ["1 · start", "2 · çalışma", "3 · finish", "4 · yeni oturum", "5 · status · inspect"]
    for x, name in zip(cols, steps):
        s.text(x + W / 2, top + 100, name, 18, INK, "middle", True)

    def card(x, y, h, key, title, lines, dashed=False):
        stroke = C[key][1]
        s.rect(x, y, W, h, "#fbfbfb" if dashed else "#ffffff", stroke, 1.5, dashed, 6)
        s.text(x + 14, y + 24, title, 15, INK, bold=True)
        for i, ln in enumerate(lines):
            s.text(x + 14, y + 46 + i * 18, ln, 12, MUTED)

    m, b = "agent", "store"
    card(cols[0], 215, 150, m, "rmk start", ["base = doğrulanmış remote SHA", "task başına worktree + branch",
                                             "ledger: execution.started", "plan task ID tam-token eşleşir"])
    card(cols[1], 215, 150, m, "Git gerçeği", ["worktree'deki diff ve commit'ler", "ledger olaylarının zaman damgası",
                                              "→ last activity (ikinci sinyal)", "PID yok, heartbeat yok"])
    card(cols[2], 215, 150, m, "rmk finish", ["commit'siz iş → refs/runmark/preserved", "worktree: exists · clean · merged",
                                             "silme önerisi yalnız clean && merged", "transcript → test koşuları + exit (ADR-021)"])
    card(cols[3], 215, 150, m, "SessionStart hook", ["rmk resume --markdown --hook", "hook-observed.json yazılır",
                                                    "gelmezse → hooks_not_observed", "(kör kalmak da bulgu)"])
    card(cols[4], 215, 150, m, "rules · evaluate", ["stale base · plansız execution", "ambiguous task ID",
                                                   "active execution + last activity", "unknown ≠ clean"])

    card(cols[1], 595, 170, b, "rmk evidence --kind test", ["özet ajanın yazdığı metin", "source: agent — beyan kalır",
                                                           "test_claim_unverified'ı", "kapatmaz; ölçüm transcript'ten",
                                                           "rmk note: decision · blocker"])
    card(cols[2], 595, 150, b, "handoff notu", ["'Agent note (weak evidence —", "unverified)' bölümü",
                                               "açık maddeler · kararlar", "plan [x] işaretleri de beyan"])
    card(cols[3], 595, 150, b, "yeni ajan okur", ["Verified ve Agent note", "ayrı başlıklarda gelir;",
                                                 "hangisine güveneceği", "görünür durumda"])
    card(cols[0], 595, 150, b, "başlatma", ["hangi task, hangi ajan", "insan/ajan seçer;",
                                           "Runmark ajanı başlatmaz", "(launch agent: ADR-020)"], True)

    # Main loop along the measured band, then back to the start.
    for a, c in zip(cols, cols[1:]):
        s.arrow(f"M{a + W},290 H{c}")
    s.arrow(f"M{cols[4] + W / 2},365 V420 H{cols[0] + W / 2} V365", "#2d7a3e",
            label="bulgu bir sonraki start'ı veya devralmayı yönlendirir — sessiz kalmaz",
            lx=(cols[0] + cols[4] + W) / 2, ly=412, anchor="middle")
    # Claims cross the boundary only as labelled text.
    for x in cols[1:4]:
        s.arrow(f"M{x + W / 2},595 V365", "#b36b00", True)
    s.arrow(f"M{cols[0] + W / 2},595 V365", "#b36b00", True)
    s.text(cols[0] + W / 2 + 8, 520, "girdi", 12, "#b36b00")
    s.text(cols[1] + W / 2 + 8, 520, "kayıt", 12, "#b36b00")
    s.text(cols[2] + W / 2 + 8, 520, "etiketli bölüm", 12, "#b36b00")
    s.text(cols[3] + W / 2 + 8, 520, "bağlama girer", 12, "#b36b00")

    s.rect(30, 810, 1640, 130, "#ffffff", "#8c959f")
    s.text(50, 842, "Okuma kuralı", 20, INK, bold=True)
    s.text(50, 872, "• Yeşil banttaki her şey Runmark'ın kendi gözlemidir; turuncu banttakiler kaydedilir ama doğrulanmaz.", 15)
    s.text(50, 896, "• Turuncudan yeşile geçen tek şey etiketli metindir: hiçbir beyan bir bulguyu tek başına kapatmamalı.", 15)
    s.text(50, 920, "• Test sonucu: ajanın yazdığı 'geçti' beyan; finish'in transcript'te bulduğu koşu ve exit kodu ölçüm (ADR-021).", 15)
    return s.render()


if __name__ == "__main__":
    out = sys.argv[1]
    open(f"{out}/runmark-architecture.svg", "w").write(architecture())
    open(f"{out}/runmark-execution-loop.svg", "w").write(loop())
