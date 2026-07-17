#include <QtTest>

#include "providers/CustomJsonParser.h"
#include "providers/GenericBalanceParser.h"
#include "providers/GlmParser.h"
#include "providers/OpenAiCostsParser.h"

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
    // ---- GLM 配额模板 ----
    void glm_parsesFiveHourAndWeekly();
    void glm_rejectsBadJson();
    void glm_rejectsErrorCode();
    void glm_skipsMalformedEntries();

    // ---- 通用余额模板（自动识别 DeepSeek / Moonshot 形态） ----
    void genericBalance_parsesDeepSeekShape();
    void genericBalance_rejectsMissingInfos();
    void genericBalance_parsesMoonshotShape();
    void genericBalance_rejectsErrorCode();

    // ---- OpenAI 费用模板 ----
    void openai_sumsBuckets();
    void openai_rejectsErrorShape();

    // ---- 自定义 JSON 映射 ----
    void custom_parsesQuotaAndBalance();
    void custom_rejectsWhenNoPaths();
};

void TestParsers::glm_parsesFiveHourAndWeekly()
{
    const QByteArray body = fixture(QStringLiteral("glm_quota.json"));
    QVERIFY2(!body.isEmpty(), "夹具 glm_quota.json 缺失");

    const ProviderSnapshot snapshot = parseGlmQuotaResponse(body, {}, QStringLiteral("glm"));
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

    QVERIFY(qAbs(snapshot.worstQuotaPercent() - 0.4) < 1e-9);
}

void TestParsers::glm_rejectsBadJson()
{
    const ProviderSnapshot snapshot =
        parseGlmQuotaResponse("{ not json", {}, QStringLiteral("glm"));
    QVERIFY(!snapshot.ok());
    QVERIFY(snapshot.error.contains(QStringLiteral("JSON")));
}

void TestParsers::glm_rejectsErrorCode()
{
    const QByteArray body = R"({"code": 401, "msg": "Unauthorized"})";
    const ProviderSnapshot snapshot = parseGlmQuotaResponse(body, {}, QStringLiteral("glm"));
    QVERIFY(!snapshot.ok());
    QVERIFY(snapshot.error.contains(QStringLiteral("401")));
}

void TestParsers::glm_skipsMalformedEntries()
{
    const QByteArray body = R"({
        "code": 200,
        "data": { "limits": [
            { "type": "TIME_LIMIT", "usage": 10, "total": 50 },
            { "type": "UNKNOWN_NEW_TYPE", "whatever": 1 }
        ] }
    })";
    const ProviderSnapshot snapshot = parseGlmQuotaResponse(body, {}, QStringLiteral("glm"));
    QVERIFY2(snapshot.ok(), qPrintable(snapshot.error));
    QCOMPARE(snapshot.quotas.size(), 1);
    QCOMPARE(snapshot.quotas.at(0).used, 10.0);
    QCOMPARE(snapshot.quotas.at(0).limit, 50.0);
}

void TestParsers::genericBalance_parsesDeepSeekShape()
{
    const QByteArray body = fixture(QStringLiteral("deepseek_balance.json"));
    QVERIFY2(!body.isEmpty(), "夹具 deepseek_balance.json 缺失");

    const ProviderSnapshot snapshot = parseGenericBalance(body, {}, QStringLiteral("deepseek"));
    QVERIFY2(snapshot.ok(), qPrintable(snapshot.error));
    QVERIFY(snapshot.api.has_value());
    QCOMPARE(snapshot.api->currency, QStringLiteral("CNY"));
    QCOMPARE(snapshot.api->balance, 53.10);
    QVERIFY(snapshot.api->note.contains(QStringLiteral("赠送")));
    QVERIFY(snapshot.quotas.isEmpty());
    QCOMPARE(snapshot.worstQuotaPercent(), -1.0);
}

void TestParsers::genericBalance_rejectsMissingInfos()
{
    const ProviderSnapshot snapshot =
        parseGenericBalance(R"({"is_available": true})", {}, QStringLiteral("deepseek"));
    QVERIFY(!snapshot.ok());
}

void TestParsers::genericBalance_parsesMoonshotShape()
{
    const QByteArray body = fixture(QStringLiteral("kimi_balance.json"));
    QVERIFY2(!body.isEmpty(), "夹具 kimi_balance.json 缺失");

    const ProviderSnapshot snapshot = parseGenericBalance(body, {}, QStringLiteral("kimi"));
    QVERIFY2(snapshot.ok(), qPrintable(snapshot.error));
    QVERIFY(snapshot.api.has_value());
    QCOMPARE(snapshot.api->balance, 56.90);
    QCOMPARE(snapshot.api->currency, QStringLiteral("CNY"));
    QVERIFY(snapshot.api->note.contains(QStringLiteral("代金券")));
}

void TestParsers::genericBalance_rejectsErrorCode()
{
    const QByteArray body =
        R"({"code": 5, "message": "Invalid Authentication", "status": false})";
    const ProviderSnapshot snapshot = parseGenericBalance(body, {}, QStringLiteral("kimi"));
    QVERIFY(!snapshot.ok());
    QVERIFY(snapshot.error.contains(QStringLiteral("Invalid Authentication")));
}

void TestParsers::openai_sumsBuckets()
{
    const QByteArray body = fixture(QStringLiteral("openai_costs.json"));
    QVERIFY2(!body.isEmpty(), "夹具 openai_costs.json 缺失");

    const ProviderSnapshot snapshot = parseOpenAiCosts(body, QStringLiteral("openai"));
    QVERIFY2(snapshot.ok(), qPrintable(snapshot.error));
    QVERIFY(snapshot.api.has_value());
    QCOMPARE(snapshot.api->label, QStringLiteral("本月花费"));
    QCOMPARE(snapshot.api->balance, 4.72);   // 1.25 + 0.47 + 3.00
    QCOMPARE(snapshot.api->currency, QStringLiteral("USD"));
}

void TestParsers::openai_rejectsErrorShape()
{
    const QByteArray body =
        R"({"error": {"message": "Invalid API key", "type": "invalid_request_error"}})";
    const ProviderSnapshot snapshot = parseOpenAiCosts(body, QStringLiteral("openai"));
    QVERIFY(!snapshot.ok());
    QVERIFY(snapshot.error.contains(QStringLiteral("Invalid API key")));
}

void TestParsers::custom_parsesQuotaAndBalance()
{
    const QByteArray body = R"({
        "usage": { "windows": [
            { "name": "5h", "consumed": 12, "cap": 60, "reset_at": 1752847200 }
        ] },
        "account": { "money": "88.5", "unit": "CNY" }
    })";
    const QJsonObject fields{
        {QStringLiteral("quotasArray"), QStringLiteral("usage.windows")},
        {QStringLiteral("used"), QStringLiteral("consumed")},
        {QStringLiteral("limit"), QStringLiteral("cap")},
        {QStringLiteral("reset"), QStringLiteral("reset_at")},
        {QStringLiteral("balance"), QStringLiteral("account.money")},
        {QStringLiteral("currency"), QStringLiteral("account.unit")},
    };

    const ProviderSnapshot snapshot =
        parseCustomJsonResponse(body, fields, QStringLiteral("user-test"));
    QVERIFY2(snapshot.ok(), qPrintable(snapshot.error));
    QCOMPARE(snapshot.providerId, QStringLiteral("user-test"));

    QCOMPARE(snapshot.quotas.size(), 1);
    QCOMPARE(snapshot.quotas.at(0).used, 12.0);
    QCOMPARE(snapshot.quotas.at(0).limit, 60.0);
    QVERIFY(snapshot.quotas.at(0).resetAt.isValid());   // 秒级 epoch 也能识别

    QVERIFY(snapshot.api.has_value());
    QCOMPARE(snapshot.api->balance, 88.5);
    QCOMPARE(snapshot.api->currency, QStringLiteral("CNY"));
}

void TestParsers::custom_rejectsWhenNoPaths()
{
    const ProviderSnapshot snapshot =
        parseCustomJsonResponse(R"({"foo": 1})", {}, QStringLiteral("user-test"));
    QVERIFY(!snapshot.ok());
}

QTEST_MAIN(TestParsers)
#include "test_parsers.moc"
