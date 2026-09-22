#include "handoff.h"

#include "error.h"
#include "runmark/workflow.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTextStream>

namespace runmark {

void readHandoff(const Paths& paths, ResumeFacts& facts)
{
    // A ledger ID is data, never a relative filesystem path.
    if (facts.exec.contains(QLatin1Char('/')) || facts.exec.contains(QStringLiteral(".."))) {
        facts.handoff.error = QStringLiteral("Invalid execution ID for handoff path");
        return;
    }
    facts.handoff = observePath(QDir(paths.handoffs).filePath(facts.exec + QStringLiteral(".md")));
    QFile file(facts.handoff.path);
    if (!file.open(QIODevice::ReadOnly)) {
        facts.handoff.error = file.errorString();
        return;
    }
    const QByteArray content = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        facts.handoff.error = file.errorString();
        return;
    }
    facts.handoff.sha1 = QString::fromLatin1(QCryptographicHash::hash(content, QCryptographicHash::Sha1).toHex());
    facts.handoffContent = QString::fromUtf8(content);
}

QJsonObject resumePackage(const ResumeFacts& facts, const QJsonArray& gaps)
{
    const auto nullable = [](const QString& value) -> QJsonValue {
        return value.isEmpty() ? QJsonValue::Null : QJsonValue(value);
    };
    const auto boolean = [](std::optional<bool> value) -> QJsonValue {
        return value.has_value() ? QJsonValue(*value) : QJsonValue::Null;
    };
    const QString recordedPlan = facts.started.value(QStringLiteral("plan_sha1")).toString();
    const QString recordedHandoff = facts.finished.value(QStringLiteral("handoff_sha1")).toString();
    QJsonArray evidence, claims, withRef, withoutRef;
    for (const QJsonObject& event : facts.events) {
        const QString kind = event.value(QStringLiteral("kind")).toString();
        if (event.value(QStringLiteral("type")) == QLatin1String("evidence.recorded")) {
            if (kind == QLatin1String("test") || kind == QLatin1String("command")) evidence.append(event);
            if (kind == QLatin1String("agent_summary")) claims.append(event);
        }
        if (event.value(QStringLiteral("type")) == QLatin1String("note") && kind == QLatin1String("unresolved")) {
            (event.value(QStringLiteral("ref")).toString().isEmpty() ? withoutRef : withRef).append(event);
        }
    }
    QJsonObject measured = facts.measured;
    for (const QString& key : {QStringLiteral("source"), QStringLiteral("commits"), QStringLiteral("files_changed")}) {
        if (!measured.contains(key)) measured.insert(key, QJsonValue::Null);
    }
    measured.insert(QStringLiteral("evidence"), evidence);
    return {{QStringLiteral("task"), facts.task}, {QStringLiteral("exec"), nullable(facts.exec)},
        {QStringLiteral("plan_ref"), nullable(facts.started.value(QStringLiteral("plan_ref")).toString())},
        {QStringLiteral("plan_sha1"), nullable(recordedPlan)},
        {QStringLiteral("current_plan_sha1"), nullable(facts.planSha1)},
        {QStringLiteral("plan_changed"), recordedPlan.isEmpty() || facts.planSha1.isEmpty()
            ? QJsonValue::Null : QJsonValue(recordedPlan != facts.planSha1)},
        {QStringLiteral("workspace"), QJsonObject{
            {QStringLiteral("worktree"), nullable(facts.started.value(QStringLiteral("worktree")).toString())},
            {QStringLiteral("branch"), nullable(facts.started.value(QStringLiteral("branch")).toString())},
            {QStringLiteral("base"), nullable(facts.started.value(QStringLiteral("base")).toString())},
            {QStringLiteral("base_sha"), nullable(facts.started.value(QStringLiteral("base_sha")).toString())},
            {QStringLiteral("remote_base_sha"), nullable(facts.started.value(QStringLiteral("remote_base_sha")).toString())},
            {QStringLiteral("current_base_sha"), nullable(facts.currentBaseSha)},
            {QStringLiteral("worktree_exists"), boolean(facts.worktree.exists)},
            {QStringLiteral("base_advanced"), boolean(facts.baseAdvanced)}}},
        {QStringLiteral("measured"), measured},
        {QStringLiteral("agent_claims"), QJsonObject{{QStringLiteral("verification"), QStringLiteral("doğrulanmadı")},
            {QStringLiteral("evidence"), claims}}},
        {QStringLiteral("unresolved"), QJsonObject{{QStringLiteral("with_ref"), withRef}, {QStringLiteral("without_ref"), withoutRef}}},
        {QStringLiteral("instructions"), facts.started.contains(QStringLiteral("instructions"))
            ? facts.started.value(QStringLiteral("instructions")) : QJsonValue::Null},
        {QStringLiteral("preserved_ref"), nullable((facts.finished.isEmpty() ? facts.started : facts.finished).value(QStringLiteral("preserved_ref")).toString())},
        {QStringLiteral("handoff"), QJsonObject{{QStringLiteral("path"), nullable(facts.handoff.path)},
            {QStringLiteral("sha1"), nullable(facts.handoff.sha1)}, {QStringLiteral("recorded_sha1"), nullable(recordedHandoff)},
            {QStringLiteral("verified"), recordedHandoff.isEmpty() || facts.handoff.sha1.isEmpty()
                ? QJsonValue::Null : QJsonValue(recordedHandoff == facts.handoff.sha1)},
            {QStringLiteral("content"), facts.handoff.sha1.isEmpty() ? QJsonValue::Null : QJsonValue(facts.handoffContent)}}},
        {QStringLiteral("gaps"), gaps}};
}

QString resumeMarkdown(const QJsonObject& package)
{
    // Bu cikti ajana yapistirilmak icin. JSON isteyen varsayilani kullanir;
    // burada handoff.md ile ayni dil konusulur, yoksa iki format ayrisir.
    QString text;
    QTextStream out(&text);

    const auto str = [](const QJsonValue& value, const QString& fallback = QStringLiteral("—")) {
        return value.isString() ? value.toString() : fallback;
    };
    // Bilinmiyor ile hayir ayri seyler: null "—" kalir, false acikca yazilir.
    const auto tri = [](const QJsonValue& value, const QString& yes, const QString& no) {
        return value.isBool() ? (value.toBool() ? yes : no) : QStringLiteral("bilinmiyor");
    };

    const QJsonValue task = package.value(QStringLiteral("task"));
    out << "# Runmark resume";
    if (task.isString() && !task.toString().isEmpty()) out << ": " << task.toString();
    out << "\n\n";

    const QJsonValue exec = package.value(QStringLiteral("exec"));
    if (!exec.isString()) {
        out << (task.isString() && !task.toString().isEmpty()
                ? "Bu task için kayıtlı execution yok.\n\n"
                : "Bu projede kayıtlı execution yok.\n\n");
    } else {
        const QJsonObject workspace = package.value(QStringLiteral("workspace")).toObject();
        out << "exec: " << exec.toString() << '\n'
            << "plan: " << str(package.value(QStringLiteral("plan_ref")))
            << "  (" << tri(package.value(QStringLiteral("plan_changed")),
                            QStringLiteral("execution'dan beri DEĞİŞTİ"), QStringLiteral("değişmedi")) << ")\n"
            << "worktree: " << str(workspace.value(QStringLiteral("worktree")))
            << "  (" << tri(workspace.value(QStringLiteral("worktree_exists")),
                            QStringLiteral("var"), QStringLiteral("DİSKTE YOK")) << ")\n"
            << "branch: " << str(workspace.value(QStringLiteral("branch"))) << '\n'
            << "base: " << str(workspace.value(QStringLiteral("base"))) << '@'
            << str(workspace.value(QStringLiteral("base_sha")))
            << "  (" << tri(workspace.value(QStringLiteral("base_advanced")),
                            QStringLiteral("o zamandan beri İLERLEDİ"), QStringLiteral("güncel")) << ")\n";
        const QJsonValue preserved = package.value(QStringLiteral("preserved_ref"));
        if (preserved.isString()) out << "saklanan iş: " << preserved.toString() << '\n';
        out << '\n';
    }

    out << "## Doğrulanmış (Runmark üretti)\n\n";
    const QJsonObject measured = package.value(QStringLiteral("measured")).toObject();
    const QJsonArray commits = measured.value(QStringLiteral("commits")).toArray();
    if (commits.isEmpty()) {
        out << "Commit yok.\n";
    } else {
        out << "Commits:\n";
        for (const QJsonValue& commit : commits) out << "- " << commit.toString() << '\n';
    }
    const QJsonValue filesChanged = measured.value(QStringLiteral("files_changed"));
    if (filesChanged.isDouble()) out << "\nDeğişen dosya: " << filesChanged.toInt() << '\n';
    const QJsonArray evidence = measured.value(QStringLiteral("evidence")).toArray();
    out << "\nKanıt:\n";
    if (evidence.isEmpty()) {
        out << "Yok.\n";
    } else {
        for (const QJsonValue& value : evidence) {
            const QJsonObject item = value.toObject();
            out << "- " << str(item.value(QStringLiteral("kind"))) << ": "
                << str(item.value(QStringLiteral("summary"))) << '\n';
        }
    }

    out << "\n## Agent notu (zayıf evidence — doğrulanmadı)\n\n";
    const QJsonArray claims = package.value(QStringLiteral("agent_claims")).toObject()
        .value(QStringLiteral("evidence")).toArray();
    if (claims.isEmpty()) {
        out << "Yok.\n";
    } else {
        for (const QJsonValue& value : claims) out << "- " << str(value.toObject().value(QStringLiteral("summary"))) << '\n';
    }

    out << "\n## Açık kalanlar\n\n";
    const QJsonObject unresolved = package.value(QStringLiteral("unresolved")).toObject();
    bool anyUnresolved = false;
    for (const QString& key : {QStringLiteral("with_ref"), QStringLiteral("without_ref")}) {
        for (const QJsonValue& value : unresolved.value(key).toArray()) {
            const QJsonObject note = value.toObject();
            const QJsonValue reference = note.value(QStringLiteral("ref"));
            out << "- [ ] " << str(note.value(QStringLiteral("text")))
                << (reference.isString() ? QStringLiteral("  (ref: ") + reference.toString() + QLatin1Char(')')
                                         : QStringLiteral("  (ref yok)")) << '\n';
            anyUnresolved = true;
        }
    }
    if (!anyUnresolved) out << "Yok.\n";

    out << "\n## Talimatlar\n\n";
    const QJsonValue instructions = package.value(QStringLiteral("instructions"));
    const QJsonArray instructionList = instructions.toArray();
    if (instructionList.isEmpty()) {
        out << (instructions.isNull() ? "Kaydedilmemiş.\n" : "Yok.\n");
    } else {
        for (const QJsonValue& value : instructionList) {
            const QJsonObject item = value.toObject();
            const QJsonValue sha1 = item.value(QStringLiteral("sha1"));
            out << "- " << str(item.value(QStringLiteral("name"))) << "  "
                << (sha1.isString() ? QStringLiteral("sha1 ") + sha1.toString().left(12)
                                    : QStringLiteral("OKUNAMADI")) << '\n';
        }
    }

    out << "\n## Handoff\n\n";
    const QJsonObject handoff = package.value(QStringLiteral("handoff")).toObject();
    const QJsonValue verified = handoff.value(QStringLiteral("verified"));
    if (!handoff.value(QStringLiteral("path")).isString()) {
        out << "Yok.\n";
    } else {
        out << "yol: " << str(handoff.value(QStringLiteral("path"))) << '\n'
            << "sha1: " << str(handoff.value(QStringLiteral("sha1"))) << '\n';
        if (!handoff.value(QStringLiteral("sha1")).isString()) {
            out << "durum: dosya okunamadı\n";
        } else if (!verified.isBool()) {
            out << "durum: doğrulanamadı (ledger'da kayıtlı sha1 yok)\n";
        } else if (verified.toBool()) {
            out << "durum: kayıtlı sha1 ile eşleşiyor\n";
        } else {
            out << "durum: DEĞİŞMİŞ — kayıtlı sha1 "
                << str(handoff.value(QStringLiteral("recorded_sha1"))) << '\n';
        }
    }

    out << "\n## Eksiklikler\n\n";
    const QJsonArray gaps = package.value(QStringLiteral("gaps")).toArray();
    if (gaps.isEmpty()) {
        out << "Yok.\n";
    } else {
        for (const QJsonValue& value : gaps) {
            const QJsonObject gap = value.toObject();
            out << "- " << str(gap.value(QStringLiteral("id"))) << " — "
                << str(gap.value(QStringLiteral("explanation"))) << '\n';
        }
    }
    return text;
}

void writeHandoff(const Paths& paths, const HandoffInput& input, const QVector<QJsonObject>& events)
{
    QFile handoff(QDir(paths.handoffs).filePath(input.executionId + QStringLiteral(".md")));
    if (!handoff.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        fail(QStringLiteral("Cannot write handoff: %1").arg(handoff.fileName()));
    }
    QTextStream output(&handoff);
    output << "---\nexec: " << input.executionId
           << "\ntask: " << input.started.value(QStringLiteral("task")).toString()
           << "\nagent: " << input.started.value(QStringLiteral("agent")).toString()
           << "\noutcome: " << input.outcome
           << "\nrepo: " << input.started.value(QStringLiteral("repo")).toString()
           << "\nworktree: " << input.worktree
           << "\nbranch: " << input.started.value(QStringLiteral("branch")).toString()
           << "\nbase: " << input.started.value(QStringLiteral("base")).toString() << "@" << input.baseSha
           << "\nrange: " << input.baseSha << ".." << input.headSha
           << "\n---\n\n## Doğrulanmış (Runmark üretti)\n\nCommits:\n";
    for (const QString& commit : input.commitLines) output << "- " << commit << '\n';
    output << "\nDeğişen dosyalar: " << input.filesChanged << " (+" << input.insertions << " / -" << input.deletions << ")\n";
    for (const QString& file : input.files) output << "- " << file << '\n';
    output << "\nEvidence: " << input.filesRef << '\n';
    if (!input.preservedRef.isEmpty()) output << "\nPreserved uncommitted snapshot: " << input.preservedRef << '\n';

    output << "\n## Agent notu (zayıf evidence — doğrulanmadı)\n\n";
    bool hasAgentSummary = false;
    for (const QJsonObject& event : events) {
        if (event.value(QStringLiteral("type")).toString() == QLatin1String("evidence.recorded")
                && event.value(QStringLiteral("exec")).toString() == input.executionId
                && event.value(QStringLiteral("kind")).toString() == QLatin1String("agent_summary")) {
            output << "- " << event.value(QStringLiteral("summary")).toString() << '\n';
            hasAgentSummary = true;
        }
    }
    if (!hasAgentSummary) output << "Yok.\n";

    output << "\n## Açık kalanlar\n\n";
    bool hasUnresolved = false;
    for (const QJsonObject& event : events) {
        if (event.value(QStringLiteral("type")).toString() == QLatin1String("note")
                && event.value(QStringLiteral("exec")).toString() == input.executionId
                && event.value(QStringLiteral("kind")).toString() == QLatin1String("unresolved")) {
            output << "- [ ] " << event.value(QStringLiteral("text")).toString();
            const QJsonValue reference = event.value(QStringLiteral("ref"));
            output << (reference.isNull() ? "  (ref yok)" : "  (ref: " + reference.toString() + ")") << '\n';
            hasUnresolved = true;
        }
    }
    if (!hasUnresolved) output << "Yok.\n";

    output.flush();
    if (output.status() != QTextStream::Ok) {
        fail(QStringLiteral("Cannot write handoff: %1").arg(handoff.fileName()));
    }
}

} // namespace runmark
