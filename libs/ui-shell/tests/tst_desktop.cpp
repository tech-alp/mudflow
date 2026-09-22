#include "StatusViewModel.h"
#include "FindingFilterModel.h"
#include <QQuickWindow>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QSettings>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTest>
#include <QThreadPool>

class DesktopTest : public QObject
{
    Q_OBJECT
private slots:
    void filtering()
    {
        runmark::FindingModel model;
        const runmark::FindingModel::Row first{"git.behind", "warning", "git", "First repository", "Repo A", "Fetch"};
        const runmark::FindingModel::Row second{"git.behind", "warning", "git", "Second repository", "Repo B", "Fetch"};
        const runmark::FindingModel::Row plan{"plan.no_tests", "info", "plan", "Tests missing", "Task 12", "Run tests"};
        model.reset({first, second, plan});
        runmark::FindingFilterModel filter;
        filter.setSourceModel(&model);
        QCOMPARE(filter.count(), 3);
        QCOMPARE(filter.domains().size(), 2);
        const auto key = model.index(1, 0).data(runmark::FindingModel::KeyRole).toString();
        filter.setSelectedKey(key);
        QCOMPARE(filter.selectedFinding().value("title").toString(), second.title);
        model.reset({plan, second, first});
        QCOMPARE(filter.selectedKey(), key);
        QCOMPARE(filter.selectedFinding().value("title").toString(), second.title);
        filter.setQuery("REPO B");
        QCOMPARE(filter.count(), 1);
        QCOMPARE(filter.selectedKey(), key);
        filter.setDomain("plan");
        QCOMPARE(filter.count(), 0);
        QVERIFY(filter.selectedKey().isEmpty());
        filter.setQuery("");
        QCOMPARE(filter.count(), 1);
        QCOMPARE(filter.totalCount(), 3);
        filter.setDomain("");
        filter.setSelectedKey(key);
        model.reset({first, plan});
        QVERIFY(filter.selectedFinding().isEmpty());
        QVERIFY(filter.selectedKey().isEmpty());
    }
    void projectWorkflow()
    {
        QCoreApplication::setOrganizationName("RunmarkTests");
        QCoreApplication::setApplicationName("DesktopWorkflow");
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, temporary.path());
        QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, temporary.path());
        const QString previousDirectory = QDir::currentPath();
        QVERIFY(QDir::setCurrent(temporary.path()));
        const auto restoreDirectory = qScopeGuard([&] { QDir::setCurrent(previousDirectory); });
        const auto waitForWorkers = qScopeGuard([] { QThreadPool::globalInstance()->waitForDone(); });
        const QString config = temporary.filePath("project.json");

        runmark::StatusViewModel view;
        view.restoreProject();
        QVERIFY(view.configPath().isEmpty());
        QVERIFY(view.error().isEmpty());
        QVERIFY(!view.busy());

        view.openProject(QUrl::fromLocalFile(config));
        QVERIFY(view.busy());
        view.setConfigPath("ignored.json");
        QCOMPARE(view.configPath(), config);
        QTRY_VERIFY_WITH_TIMEOUT(!view.busy(), 5000);
        QVERIFY(!view.error().isEmpty());
        QVERIFY(QSettings().value("desktop/lastProject").toString().isEmpty());

        QFile file(config);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(R"({"version":1,"name":"desktop-test","worktree_root":"worktrees","repos":[{"name":"missing","path":"missing","base":{"remote":"origin","branch":"main"}}],"plan":{"path":"plan.md"},"task_id_pattern":"T-\\d+"})");
        file.close();
        view.refresh();
        QTRY_VERIFY_WITH_TIMEOUT(!view.busy(), 5000);
        QCOMPARE(view.error(), QString());
        QCOMPARE(view.project(), QString("desktop-test"));
        QCOMPARE(QSettings().value("desktop/lastProject").toString(), config);

        runmark::StatusViewModel restored;
        restored.restoreProject();
        QCOMPARE(restored.configPath(), config);
        QTRY_VERIFY_WITH_TIMEOUT(!restored.busy(), 5000);
        QCOMPARE(restored.project(), QString("desktop-test"));

        restored.openProject(QUrl::fromLocalFile(temporary.filePath("missing.json")));
        QTRY_VERIFY_WITH_TIMEOUT(!restored.busy(), 5000);
        QVERIFY(!restored.error().isEmpty());
        QVERIFY(restored.project().isEmpty());
        QCOMPARE(restored.findings()->rowCount(), 0);
        QCOMPARE(QSettings().value("desktop/lastProject").toString(), config);

        // Explicit launch path takes precedence over remembered project.
        runmark::StatusViewModel explicitLaunch;
        explicitLaunch.restoreProject(temporary.filePath("explicit.json"));
        QVERIFY(explicitLaunch.configPath().endsWith("explicit.json"));
        QTRY_VERIFY_WITH_TIMEOUT(!explicitLaunch.busy(), 5000);
        QVERIFY(!explicitLaunch.error().isEmpty());

        // Without a saved project, a project in the working directory is usable.
        QSettings().remove("desktop/lastProject");
        QVERIFY(QDir().mkpath(".runmark"));
        QVERIFY(QFile::copy(config, ".runmark/project.json"));
        runmark::StatusViewModel local;
        local.restoreProject();
        QCOMPARE(QFileInfo(local.configPath()).canonicalFilePath(),
            QFileInfo(temporary.filePath(".runmark/project.json")).canonicalFilePath());
        QTRY_VERIFY_WITH_TIMEOUT(!local.busy(), 5000);
        QCOMPARE(local.project(), QString("desktop-test"));
        QSettings().setValue("desktop/lastProject", config);
        runmark::StatusViewModel remembered;
        remembered.restoreProject();
        QCOMPARE(remembered.configPath(), config);
        QTRY_VERIFY_WITH_TIMEOUT(!remembered.busy(), 5000);

        auto* closing = new runmark::StatusViewModel;
        closing->openProject(QUrl::fromLocalFile(config));
        delete closing;
        QVERIFY(QThreadPool::globalInstance()->waitForDone(5000));
        QCoreApplication::processEvents();

        QQmlApplicationEngine engine;
        engine.setInitialProperties({
            {"configPath", temporary.filePath("missing.json")},
            {"themeIndexPath", QStringLiteral(DESKTOP_THEME_INDEX)}});
        engine.loadFromModule("Runmark.Shell", "Shell");
        QVERIFY(!engine.rootObjects().isEmpty());
        auto* root = engine.rootObjects().first();
        auto* error = root->findChild<QQuickItem*>("projectError");
        auto* toolbar = root->findChild<QQuickItem*>("projectToolbar");
        QVERIFY(error);
        QVERIFY(toolbar);
        QTRY_VERIFY_WITH_TIMEOUT(error->isVisible() && error->height() > 0, 5000);
        for (const int width : {1000, 480}) {
            root->setProperty("width", width);
            QTest::qWait(100);
            QVERIFY(error->mapToScene(QPointF()).y()
                    >= toolbar->mapToScene(QPointF(0, toolbar->height())).y());
            QVERIFY(error->mapToScene(QPointF(error->width(), 0)).x() <= width);
        }

        auto* window = qobject_cast<QQuickWindow*>(root);
        auto* status = root->findChild<runmark::StatusViewModel*>();
        auto* filter = root->findChild<runmark::FindingFilterModel*>();
        auto* search = root->findChild<QQuickItem*>("findingsSearch");
        auto* details = root->findChild<QQuickItem*>("findingDetails");
        auto* list = root->findChild<QQuickItem*>("findingsList");
        QVERIFY(window && status && filter && search && details && list);
        status->openProject(QUrl::fromLocalFile(config));
        QTRY_VERIFY_WITH_TIMEOUT(!status->busy(), 5000);
        QVERIFY(status->error().isEmpty());
        status->findings()->reset({
            {"plan.no_tests", "warning", "plan", "Test kanıtı eksik", "Plan maddesi için henüz test sonucu kaydedilmedi.", "Testleri çalıştırın ve sonucu kaydedin."},
            {"git.behind", "warning", "git", "Çalışma dalı geride", "Repo A ana dalın gerisinde.", "Değişiklikleri inceleyin."},
            {"git.behind", "info", "git", "İkinci çalışma alanı", "Repo B ölçümü.", ""}});
        root->setProperty("width", 1440);
        root->setProperty("height", 900);
        QTest::qWait(150);
        search->setProperty("text", "Repo B");
        QTRY_COMPARE(filter->count(), 1);
        search->setProperty("text", "");
        QTRY_COMPARE(filter->count(), 3);
        filter->setSelectedKey(filter->keyAt(0));
        QTRY_VERIFY(details->isVisible());
        QDir().mkpath(QStringLiteral(DESKTOP_CAPTURE_DIR));
        QTest::qWait(200);
        QVERIFY(window->grabWindow().save(QStringLiteral(DESKTOP_CAPTURE_DIR "/findings-wide.png")));
        QVERIFY(QMetaObject::invokeMethod(toolbar, "themeRequested"));
        QTest::qWait(200);
        QVERIFY(window->grabWindow().save(QStringLiteral(DESKTOP_CAPTURE_DIR "/findings-light.png")));
        QVERIFY(QMetaObject::invokeMethod(toolbar, "themeRequested"));
        root->setProperty("width", 480);
        QTest::qWait(200);
        QVERIFY(!list->isVisible());
        QVERIFY(details->isVisible());
        QVERIFY(details->mapToScene(QPointF(details->width(), 0)).x() <= 480);
        QVERIFY(window->grabWindow().save(QStringLiteral(DESKTOP_CAPTURE_DIR "/findings-narrow.png")));
        QVERIFY(QMetaObject::invokeMethod(details, "backRequested"));
        QTRY_VERIFY(list->isVisible());
        QVERIFY(filter->selectedKey().isEmpty());
        QVERIFY(list->hasActiveFocus());
        list->setProperty("currentIndex", 0);
        QTest::keyClick(window, Qt::Key_Return);
        QTRY_VERIFY(!filter->selectedKey().isEmpty());
        QTest::keyClick(window, Qt::Key_Escape);
        QTRY_VERIFY(filter->selectedKey().isEmpty());
    }
};

QTEST_MAIN(DesktopTest)
#include "tst_desktop.moc"
