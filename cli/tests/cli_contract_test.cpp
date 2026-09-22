#include <QFile>
#include <QDir>
#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QTemporaryDir>
#include <QTextStream>
#include <stdexcept>

namespace {

bool writeFile(const QString& path, const QByteArray& contents)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(contents) == contents.size();
}

bool hasError(const QByteArray& output, const QString& code)
{
    const QJsonDocument document = QJsonDocument::fromJson(output);
    return document.isObject() && document.object().value(QStringLiteral("error")).toObject().value(QStringLiteral("code")).toString() == code;
}

bool run(const QString& executable, const QStringList& arguments, int expectedExitCode, QByteArray* standardOutput, QByteArray* standardError)
{
    QProcess process;
    process.start(executable, arguments);
    return process.waitForStarted(5000)
        && process.waitForFinished(5000)
        && process.exitCode() == expectedExitCode
        && ((*standardOutput = process.readAllStandardOutput()), true)
        && ((*standardError = process.readAllStandardError()), true);
}

void check(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

QByteArray readFile(const QString& path)
{
    QFile file(path);
    check(file.open(QIODevice::ReadOnly), "read fixture file");
    return file.readAll();
}

bool hasFinding(const QJsonObject& status, const QString& id)
{
    for (const QJsonValue& value : status.value(QStringLiteral("findings")).toArray()) {
        if (value.toObject().value(QStringLiteral("id")) == id) return true;
    }
    return false;
}

bool hasGap(const QJsonObject& package, const QString& id)
{
    for (const QJsonValue& value : package.value(QStringLiteral("gaps")).toArray()) {
        const QJsonObject gap = value.toObject();
        for (const QString& key : {QStringLiteral("id"), QStringLiteral("severity"), QStringLiteral("domain"), QStringLiteral("title"), QStringLiteral("explanation")}) {
            check(gap.value(key).isString(), "gap schema");
        }
        if (gap.value(QStringLiteral("id")) == id) return true;
    }
    return false;
}

void resumeContract(const QString& executable)
{
    QTemporaryDir fixture;
    check(fixture.isValid(), "fixture directory");
    const QString root = fixture.path();
    const QString repo = root + "/repo";
    const QString remote = root + "/remote.git";
    const QString configPath = root + "/.runmark/project.json";
    const auto git = [](const QStringList& args) {
        QByteArray out, err;
        check(run(QStringLiteral("git"), args, 0, &out, &err), "git fixture command");
        return QString::fromUtf8(out).trimmed();
    };
    const auto cli = [&](const QStringList& args) {
        QByteArray out, err;
        check(run(executable, QStringList{"--project", configPath} + args, 0, &out, &err), "CLI exit 0");
        check(err.isEmpty(), "CLI success stderr empty");
        const QJsonDocument doc = QJsonDocument::fromJson(out);
        check(doc.isObject(), "CLI stdout JSON object");
        return doc.object();
    };
    git({"init", "--bare", remote});
    git({"init", "-b", "main", repo});
    git({"-C", repo, "config", "user.name", "Runmark Contract"});
    git({"-C", repo, "config", "user.email", "test@example.invalid"});
    check(writeFile(repo + "/file.txt", "initial\n"), "initial file");
    git({"-C", repo, "add", "."});
    git({"-C", repo, "commit", "-m", "initial"});
    git({"-C", repo, "remote", "add", "origin", remote});
    git({"-C", repo, "push", "-u", "origin", "main"});
    check(QDir().mkpath(root + "/.runmark"), "state directory");
    check(writeFile(root + "/plan.md", "- [ ] MF-1\n- [ ] MF-2\n"), "plan");
    check(writeFile(root + "/AGENTS.md", "Project instructions\n"), "project instructions");
    check(writeFile(root + "/extra.md", "Extra instructions\n"), "extra instructions");
    QJsonObject config = QJsonDocument::fromJson(R"({"version":1,"name":"test","worktree_root":"worktrees","repos":[{"name":"repo","path":"repo","base":{"remote":"origin","branch":"main"}}],"plan":{"path":"plan.md"},"task_id_pattern":"MF-\\d+","instructions":["AGENTS.md"]})").object();
    check(writeFile(configPath, QJsonDocument(config).toJson()), "config");

    const QJsonObject none = cli({"resume", "MF-99"});
    check(none.value("exec").isNull() && hasGap(none, "context.no_execution"), "no execution gap");
    check(none.value("workspace").toObject().value("worktree_exists").isNull(), "no execution is unknown workspace");
    check(!QFile::exists(root + "/.runmark/ledger"), "resume creates no ledger");

    const QJsonObject started = cli({"start", "MF-1", "--agent", "claude", "--instruction", "extra.md", "--instruction", "missing.md"});
    check(started.value("warnings").toArray().size() == 1, "unreadable instruction warning");
    check(started.value("warnings").toArray().at(0).toObject().value("id") == "context.instruction_unreadable", "instruction warning id");
    const QString exec = started.value("exec").toString();
    const QString worktree = started.value("worktree").toString();
    const QString ledger = root + "/.runmark/ledger/" + exec + ".jsonl";
    const QJsonObject event = QJsonDocument::fromJson(readFile(ledger).trimmed()).object();
    const QJsonArray instructions = event.value("instructions").toArray();
    check(instructions.size() == 3, "project plus repeatable CLI instructions");
    check(instructions.at(0).toObject().value("name") == "AGENTS.md"
        && instructions.at(1).toObject().value("path") == root + "/extra.md"
        && instructions.at(2).toObject().value("sha1").isNull(), "instruction provenance schema");
    const auto hash = [](const QByteArray& content) { return QString::fromLatin1(QCryptographicHash::hash(content, QCryptographicHash::Sha1).toHex()); };
    check(instructions.at(0).toObject().value("sha1") == hash(readFile(root + "/AGENTS.md")), "instruction hash");
    check(writeFile(worktree + "/file.txt", "changed\n"), "worktree edit");
    git({"-C", worktree, "add", "."});
    git({"-C", worktree, "commit", "-m", "MF-1 implementation"});
    const QJsonObject active = cli({"resume", exec});
    check(active.value("measured").toObject().value("source") == "git"
        && active.value("measured").toObject().value("commits").toArray().size() == 1
        && active.value("measured").toObject().value("files_changed").toInt() == 1
        && hasGap(active, "context.no_handoff"), "active execution measured");
    cli({"evidence", exec, "--kind", "test", "--summary", "1 passed", "--ref", "test.log"});
    cli({"evidence", exec, "--kind", "command", "--summary", "build passed"});
    cli({"evidence", exec, "--kind", "agent_summary", "--summary", "CLAIM ONLY"});
    cli({"note", exec, "--kind", "unresolved", "--text", "No ref"});
    cli({"note", exec, "--kind", "unresolved", "--text", "Has ref", "--ref", "plan.md#L1"});
    check(writeFile(worktree + "/uncommitted.txt", "preserved\n"), "preserved file");
    cli({"finish", exec});
    const QByteArray before = readFile(ledger);
    const QJsonObject package = cli({"resume", "MF-1"});
    check(package.value("exec") == exec && package.value("task") == "MF-1", "task selects execution");
    check(cli({"resume", exec}) == package, "exec selects same package");
    const QJsonObject measured = package.value("measured").toObject();
    check(measured.value("source") == "execution.finished" && measured.value("commits").toArray().size() == 1
        && measured.value("files_changed").toInt() == 1 && measured.value("evidence").toArray().size() == 2, "finished measurements");
    check(!QJsonDocument(measured).toJson().contains("CLAIM ONLY"), "claims never measured");
    check(package.value("agent_claims").toObject().value("verification") == QStringLiteral("doğrulanmadı")
        && package.value("agent_claims").toObject().value("evidence").toArray().size() == 1, "claims explicitly unverified");
    check(package.value("unresolved").toObject().value("with_ref").toArray().size() == 1
        && package.value("unresolved").toObject().value("without_ref").toArray().size() == 1, "unresolved refs separated");
    check(package.value("instructions") == instructions, "resume recorded instructions");
    check(package.value("preserved_ref").toString().startsWith("refs/runmark/preserved/"), "preserved ref");
    const QJsonObject handoff = package.value("handoff").toObject();
    const QString handoffPath = handoff.value("path").toString();
    check(handoff.value("sha1") == hash(readFile(handoffPath)) && handoff.value("verified") == true, "handoff production hash verified");
    check(handoff.value("content").toString().toUtf8() == readFile(handoffPath), "handoff content");
    check(package.value("plan_changed") == false && package.value("workspace").toObject().value("base_advanced") == false, "known unchanged");
    QByteArray markdown, stderrOutput;
    check(run(executable, {"--project", configPath, "resume", exec, "--markdown"}, 0, &markdown, &stderrOutput)
        && stderrOutput.isEmpty() && markdown.startsWith("# Runmark resume:")
        && markdown.contains(QStringLiteral("Agent notu (zayıf evidence — doğrulanmadı)").toUtf8()) && markdown.contains(handoff.value("sha1").toString().toUtf8()), "markdown contract");
    check(readFile(ledger) == before, "resume never appends delivery event");

    // --hook: hook'un calistigi ledger'a degil ayri bir dosyaya yazilir; bu
    // olmadan hic calismamis bir hook temiz projeden ayirt edilemez.
    const QString hookObserved = root + "/.runmark/hook-observed.json";
    check(!QFile::exists(hookObserved), "no observation before --hook");
    QJsonObject hooked = config;
    hooked.insert("hooks_expected", true);
    check(writeFile(configPath, QJsonDocument(hooked).toJson()), "hooks_expected config");
    check(hasFinding(cli({"status"}), "context.hooks_not_observed"), "hook blindness reported");
    cli({"resume", exec, "--hook"});
    check(QFile::exists(hookObserved), "--hook records the observation");
    check(readFile(ledger) == before, "--hook appends no ledger event");
    check(!hasFinding(cli({"status"}), "context.hooks_not_observed"), "observation clears the finding");
    check(writeFile(configPath, QJsonDocument(config).toJson()), "restore config");

    check(writeFile(handoffPath, "changed handoff\n"), "tamper handoff");
    check(hasGap(cli({"resume", exec}), "context.handoff_changed"), "tampered handoff gap");
    check(QFile::remove(handoffPath), "delete handoff");
    const QJsonObject missing = cli({"resume", exec});
    check(hasGap(missing, "context.no_handoff") && missing.value("handoff").toObject().value("sha1").isNull(), "deleted handoff gap and null hash");
    check(writeFile(root + "/plan.md", "- [ ] MF-1 changed\n"), "change plan");
    git({"-C", repo, "commit", "--allow-empty", "-m", "advance base"});
    git({"-C", repo, "push"});
    const QJsonObject advanced = cli({"resume", exec});
    check(hasGap(advanced, "git.base_advanced") && hasGap(advanced, "plan.changed_during_execution")
        && advanced.value("workspace").toObject().value("base_advanced") == true && advanced.value("plan_changed") == true, "base and plan changes");
    git({"-C", repo, "remote", "set-url", "origin", root + "/missing.git"});
    check(QFile::remove(root + "/plan.md"), "delete plan");
    const QJsonObject offline = cli({"resume", exec});
    check(hasGap(offline, "git.fetch_failed") && offline.value("workspace").toObject().value("base_advanced").isNull()
        && hasGap(offline, "plan.comparison_unknown") && offline.value("plan_changed").isNull(), "unknown differs from false");
    git({"-C", repo, "remote", "set-url", "origin", remote});
    git({"-C", repo, "worktree", "remove", "--force", worktree});
    const QJsonObject removed = cli({"resume", exec});
    check(hasGap(removed, "git.worktree_missing") && removed.value("workspace").toObject().value("worktree_exists") == false
        && removed.value("measured") == measured, "missing worktree keeps recorded measurements");
    check(readFile(ledger) == before, "all resume cases leave ledger unchanged");

    config.remove("instructions");
    check(writeFile(configPath, QJsonDocument(config).toJson()), "no instructions config");
    const QJsonObject second = cli({"start", "MF-2"});
    const QString secondExec = second.value("exec").toString();
    const QJsonValue emptyInstructions = cli({"resume", secondExec}).value("instructions");
    check(emptyInstructions.isArray() && emptyInstructions.toArray().isEmpty(), "empty instruction list");
    git({"-C", repo, "worktree", "remove", second.value("worktree").toString()});
    const QJsonObject missingActive = cli({"resume", secondExec});
    check(hasGap(missingActive, "git.measurement_unavailable") && missingActive.value("measured").toObject().value("commits").isNull(), "missing active measurements unknown");

    const QString adoptedPath = root + "/worktrees/MF-3";
    git({"-C", repo, "worktree", "add", "-b", "external/MF-3", adoptedPath, started.value("base_sha").toString()});
    const QJsonObject adopted = cli({"start", "MF-3"});
    const QJsonObject adoptedResume = cli({"resume", adopted.value("exec").toString()});
    const QJsonObject workspace = adoptedResume.value("workspace").toObject();
    check(workspace.value("base_sha") != workspace.value("remote_base_sha") && workspace.value("base_advanced") == false,
        "adopted merge-base age is not advancement since start");
    git({"-C", remote, "update-ref", "-d", "refs/heads/main"});
    const QJsonObject deletedBase = cli({"resume", adopted.value("exec").toString()});
    check(hasGap(deletedBase, "git.fetch_failed") && deletedBase.value("workspace").toObject().value("base_advanced").isNull(),
        "deleted remote branch is not unchanged stale tracking ref");
    git({"-C", repo, "push", "origin", "main"});
    const QString baseBeforeRewrite = git({"-C", repo, "rev-parse", "main"});
    git({"-C", remote, "update-ref", "refs/heads/main", started.value("base_sha").toString()});
    const QJsonObject rewritten = cli({"resume", adopted.value("exec").toString()});
    check(hasGap(rewritten, "git.base_unknown") && rewritten.value("workspace").toObject().value("base_advanced").isNull(), "rewritten base not forward advancement");
    git({"-C", remote, "update-ref", "refs/heads/main", baseBeforeRewrite});

    // Synthetic legacy entries check selection independently of one-second IDs.
    const QString oldExec = "20000101T000000Z-MF-7";
    const QString newExec = "20000102T000000Z-MF-7";
    for (const QString& id : {oldExec, newExec}) {
        QJsonObject legacy = event;
        legacy.insert("exec", id);
        legacy.insert("task", "MF-7");
        legacy.remove("instructions");
        legacy.remove("remote_base_sha");
        legacy.insert("workspace_source", "adopted");
        check(writeFile(root + "/.runmark/ledger/" + id + ".jsonl", QJsonDocument(legacy).toJson(QJsonDocument::Compact) + '\n'), "legacy ledger");
    }
    check(cli({"resume", "MF-7"}).value("exec") == newExec && cli({"resume", oldExec}).value("exec") == oldExec, "latest task and exact exec priority");
    const QJsonObject legacy = cli({"resume", oldExec});
    check(legacy.value("instructions").isNull() && hasGap(legacy, "context.instructions_unknown")
        && legacy.value("workspace").toObject().value("base_advanced").isNull(), "legacy unknown provenance and adopted baseline");
    check(writeFile(root + "/.runmark/ledger/broken.jsonl", "not json\n"), "corrupt ledger");
    const QJsonObject corrupt = cli({"resume", "MF-99"});
    check(hasGap(corrupt, "context.ledger_unreadable") && !hasGap(corrupt, "context.no_execution"), "unreadable history not absent history");
    QTextStream(stdout) << "resume contract: git chain, no execution, missing/tampered handoff, missing worktree, offline, plan/base changes, provenance, legacy selection, ledger immutability passed\n";
}

} // namespace

int main(int argc, char* argv[])
{
    if (argc != 2) return 1;
    const QString executable = QString::fromLocal8Bit(argv[1]);
    QByteArray standardOutput;
    QByteArray standardError;

    if (!run(executable, {QStringLiteral("nonsense")}, 2, &standardOutput, &standardError)
            || !standardOutput.isEmpty()
            || !hasError(standardError, QStringLiteral("usage"))) return 1;

    if (!run(executable, {QStringLiteral("--project"), QStringLiteral("/does/not/exist/project.json"), QStringLiteral("status")}, 1, &standardOutput, &standardError)
            || !standardOutput.isEmpty()
            || !hasError(standardError, QStringLiteral("runtime"))) return 1;

    QTemporaryDir directory;
    if (!directory.isValid()
            || !writeFile(directory.filePath(QStringLiteral("project.json")), R"({"version":1,"name":"test","worktree_root":"worktrees","repos":[{"name":"repo","path":".","base":{"remote":"origin","branch":"main"}}],"plan":{"path":"plan.md"},"task_id_pattern":"MF-\\d+"})")) return 1;
    if (!run(executable, {QStringLiteral("--project"), directory.filePath(QStringLiteral("project.json")), QStringLiteral("inspect")}, 0, &standardOutput, &standardError)
            || !standardError.isEmpty()
            || !QJsonDocument::fromJson(standardOutput).isObject()) return 1;

    try {
        resumeContract(executable);
    } catch (const std::exception& error) {
        QTextStream(stderr) << error.what() << '\n';
        return 1;
    }
    return 0;
}
