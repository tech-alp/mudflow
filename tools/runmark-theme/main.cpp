#include "BrandDerivation.h"
#include "ThemeValidator.h"
#include "cpp/palettes/tones.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QSaveFile>
#include <QDebug>

using material_color_utilities::TonalPalette;

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    const auto args = app.arguments();
    if (args.size() != 4 || (args[1] != "--write" && args[1] != "--check")) {
        qCritical() << "Usage: runmark-theme --write|--check brand.json theme-directory";
        return 2;
    }
    QFile input(args[2]);
    if (!input.open(QIODevice::ReadOnly)) return 2;
    const auto brand = QJsonDocument::fromJson(input.readAll()).object();
    const auto brandValidation = ThemeValidator::validateTenantBrand(brand);
    if (!brandValidation.ok || brand.value("brandId") != "runmark") {
        qCritical() << "Invalid Runmark tenant-brand document";
        return 2;
    }
    const QColor seed(brand.value("seed").toString());
    const TonalPalette accent(seed.rgba());
    const TonalPalette neutral(accent.get_hue(), 8);
    const auto tone = [](const TonalPalette &palette, double value) {
        return QColor::fromRgba(palette.get(value)).name().toUpper();
    };
    QList<QPair<QString, QByteArray>> documents;
    for (const bool dark : {false, true}) {
        auto derived = BrandDerivation::derive(seed, dark ? BrandMode::Dark : BrandMode::Light);
        if (!derived.ok) {
            qCritical() << derived.errorCode << derived.errorMessage;
            return 1;
        }
        auto colors = derived.colors;
        const auto n = [&](double light, double night) { return tone(neutral, dark ? night : light); };
        const auto a = [&](double light, double night) { return tone(accent, dark ? night : light); };
        colors["surface"] = QJsonObject{
            {"canvas", n(97, 8)}, {"container", n(100, 12)},
            {"containerRaised", n(100, 17)}, {"containerSunken", n(94, 5)},
            {"containerTinted", n(94, 18)}, {"floating", n(100, 17)},
            {"scrim", "#A3000000"}, {"inverse", n(12, 97)}, {"shadow", "#000000"}};
        colors["content"] = QJsonObject{
            {"primary", n(12, 95)}, {"secondary", n(35, 78)},
            {"tertiary", n(40, 70)}, {"disabled", n(55, 50)},
            {"inverse", n(95, 12)}, {"link", a(38, 78)}};
        // Info stays cyan, separate from the blue brand action (Merce proximity rule).
        auto status = colors["status"].toObject();
        status["info"] = dark
            ? QJsonObject{{"container", "#063441"}, {"content", "#9DDCEC"}, {"outline", "#9DDCEC"}}
            : QJsonObject{{"container", "#E4F4F8"}, {"content", "#00566B"}, {"outline", "#00566B"}};
        status["neutral"] = QJsonObject{
            {"container", n(94, 17)}, {"content", n(35, 85)}, {"outline", n(35, 85)}};
        colors["status"] = status;
        auto action = colors["action"].toObject();
        action["primary"] = QJsonObject{
            {"container", a(42, 76)}, {"content", a(100, 12)}, {"outline", a(42, 76)}};
        action["secondary"] = QJsonObject{
            {"container", n(35, 78)}, {"content", n(100, 12)}, {"outline", n(35, 78)}};
        colors["action"] = action;
        colors["outline"] = QJsonObject{
            {"subtle", n(86, 28)}, {"strong", n(50, 60)}, {"focus", a(38, 78)}};
        const QString mode = dark ? "dark" : "light";
        const QJsonObject document{
            {"kind", "resolved-theme"}, {"resolvedThemeSchemaVersion", 1},
            {"brandId", "runmark"}, {"mode", mode},
            {"identity", QJsonObject{{"mark", seed.name().toUpper()}}},
            {"colors", colors}, {"state", derived.state}};
        const auto validation = ThemeValidator::validateResolvedTheme(document);
        if (!validation.ok) {
            for (const auto &error : validation.errors)
                qCritical() << mode << error.code << error.path << error.message;
            return 1;
        }
        const QByteArray data = QJsonDocument(document).toJson(QJsonDocument::Indented);
        const QString path = QDir(args[3]).filePath("runmark." + mode + ".json");
        documents.append({path, data});
    }
    // Validate both modes before replacing either generated artifact.
    for (const auto &[path, data] : documents) {
        if (args[1] == "--check") {
            QFile existing(path);
            if (!existing.open(QIODevice::ReadOnly) || existing.readAll() != data) {
                qCritical() << "Stale or missing theme:" << path;
                return 1;
            }
        } else {
            QSaveFile output(path);
            if (!output.open(QIODevice::WriteOnly) || output.write(data) != data.size()
                    || !output.commit()) return 1;
        }
        qInfo().noquote() << "Validated:" << path;
    }
}
