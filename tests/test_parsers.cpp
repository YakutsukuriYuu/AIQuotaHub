#include <QtTest>

#include "providers/DeepSeekParser.h"
#include "providers/GlmParser.h"

class TestParsers : public QObject
{
    Q_OBJECT

private:
    static QByteArray fixture(const QString &name)
    {
        QFile file(QCoreApplication::applicationDirPath()
                   + QStringLiteral("/fixtures/") + name);
        if (!file.open(QIODevice::ReadOnly))
            return {};
        return file.readAll();
    }

private slots:
    void glm_parsesFiveHourAndWeekly();
    void glm_rejectsBadJson();
    void glm_rejectsErrorCode();
    void glm_skipsMalformedEntries();
    void deepseek_parsesBalance();
    void deepseek_rejectsMissingInfos();
};

void TestParsers::glm_parsesFiveHourAndWeekly()
{
    const QByteArray body = fixture(QStringLiteral("glm_quota.json"));
    QVERIFY2(!body.isEmpty(), "夹具 glm_quota.json 缺失");

    const ProviderSnapshot snapshot = parseGlmQuotaResponse(body, {});
    QVERIFY2(snapshot.ok(), qPrintable(snapshot.error));
    QCOMPARE(snapshot.providerId, QStringLiteral("glm"));
    QCOMPARE(snapshot.quotas.size(), 2);

    const QuotaWindow &fiveHour = snapshot.quotas.at(0);
    QCOMPARE(fiveHour.kind, QuotaWindow::Kind::FiveHour);
    QCOMPARE(fiveHour.label, QStringLiteral("5小时额度"));
    QCOMPARE(fiveHour.used, 32.0);
    QCOMPARE(fiveHour.limit, 80.0);
    QVERIFY(qAbs(fiveHour.percent() - 0.4) < 1e-9);
    QVERIFY(fiveHour.resetAt.isValid());

    const QuotaWindow &weekly = snapshot.quotas.at(1);
    QCOMPARE(weekly.kind, QuotaWindow::Kind::Weekly);
    QCOMPARE(weekly.used, 210.0);
    QCOMPARE(weekly.limit, 600.0);

    // 最紧张的是 5 小时窗（40% > 35%）
    QVERIFY(qAbs(snapshot.worstQuotaPercent() - 0.4) < 1e-9);
}

void TestParsers::glm_rejectsBadJson()
{
    const ProviderSnapshot snapshot = parseGlmQuotaResponse("{ not json", {});
    QVERIFY(!snapshot.ok());
    QVERIFY(snapshot.error.contains(QStringLiteral("JSON")));
}

void TestParsers::glm_rejectsErrorCode()
{
    const QByteArray body = R"({"code": 401, "msg": "Unauthorized"})";
    const ProviderSnapshot snapshot = parseGlmQuotaResponse(body, {});
    QVERIFY(!snapshot.ok());
    QVERIFY(snapshot.error.contains(QStringLiteral("401")));
}

void TestParsers::glm_skipsMalformedEntries()
{
    // 字段名漂移时跳过坏条目而不是整单失败
    const QByteArray body = R"({
        "code": 200,
        "data": { "limits": [
            { "type": "TIME_LIMIT", "usage": 10, "total": 50 },
            { "type": "UNKNOWN_NEW_TYPE", "whatever": 1 }
        ] }
    })";
    const ProviderSnapshot snapshot = parseGlmQuotaResponse(body, {});
    QVERIFY2(snapshot.ok(), qPrintable(snapshot.error));
    QCOMPARE(snapshot.quotas.size(), 1);
    QCOMPARE(snapshot.quotas.at(0).used, 10.0);
    QCOMPARE(snapshot.quotas.at(0).limit, 50.0);
}

void TestParsers::deepseek_parsesBalance()
{
    const QByteArray body = fixture(QStringLiteral("deepseek_balance.json"));
    QVERIFY2(!body.isEmpty(), "夹具 deepseek_balance.json 缺失");

    const ProviderSnapshot snapshot = parseDeepSeekBalance(body);
    QVERIFY2(snapshot.ok(), qPrintable(snapshot.error));
    QVERIFY(snapshot.api.has_value());
    QCOMPARE(snapshot.api->currency, QStringLiteral("CNY"));
    QCOMPARE(snapshot.api->balance, 53.10);
    QVERIFY(snapshot.api->note.contains(QStringLiteral("赠送")));
    QVERIFY(snapshot.quotas.isEmpty());
    QCOMPARE(snapshot.worstQuotaPercent(), -1.0);
}

void TestParsers::deepseek_rejectsMissingInfos()
{
    const ProviderSnapshot snapshot = parseDeepSeekBalance(R"({"is_available": true})");
    QVERIFY(!snapshot.ok());
}

QTEST_MAIN(TestParsers)
#include "test_parsers.moc"
