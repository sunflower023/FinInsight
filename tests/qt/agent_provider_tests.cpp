#include "analysis/OpenAICompatibleReviewGenerator.h"
#include <QHostAddress>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>
class AgentProviderTests final : public QObject {
    Q_OBJECT
private slots:
    void generatesAndRejectsInvalidResponse();
    void missingConfigDoesNotUseNetwork();
};
void AgentProviderTests::generatesAndRejectsInvalidResponse()
{
    QTcpServer server; QVERIFY(server.listen(QHostAddress::LocalHost,0)); QByteArray captured; bool invalid=false;
    connect(&server,&QTcpServer::newConnection,this,[&]{ auto* socket=server.nextPendingConnection(); connect(socket,&QTcpSocket::readyRead,socket,[&,socket]{ captured+=socket->readAll();
        const int headerEnd=captured.indexOf("\r\n\r\n"); if(headerEnd<0)return; const auto headers=captured.left(headerEnd); int length=0;
        for(const auto& line:headers.split('\n')) if(line.toLower().startsWith("content-length:")) length=line.mid(15).trimmed().toInt(); if(captured.size()<headerEnd+4+length)return;
        const QByteArray body=invalid?QByteArray("{}"):QByteArray(R"({"choices":[{"message":{"content":"Facts: [CONCENTRATION] based on trade 1."}}]})");
        socket->write("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: "+QByteArray::number(body.size())+"\r\nConnection: close\r\n\r\n"+body); socket->disconnectFromHost(); }); });
    fininsight::analysis::OpenAICompatibleReviewGenerator provider({QStringLiteral("http://127.0.0.1:%1/v1/chat/completions").arg(server.serverPort()),QStringLiteral("secret"),QStringLiteral("test-model"),1000,300});
    fininsight::analysis::EvidenceSnapshot evidence; evidence.trades.push_back({1,fininsight::simulation::TradeSide::Buy,"AAPL",1,100,0,1,0});
    fininsight::analysis::BehaviorReport report; report.findings.push_back({"CONCENTRATION",fininsight::analysis::FindingSeverity::Warning,"Concentrated",1.0,0,1,{1}});
    QSignalSpy completed(&provider,&fininsight::analysis::OpenAICompatibleReviewGenerator::completed); QSignalSpy failed(&provider,&fininsight::analysis::OpenAICompatibleReviewGenerator::failed);
    provider.generateAsync(evidence,report); QTRY_COMPARE_WITH_TIMEOUT(completed.count(),1,2000); QCOMPARE(failed.count(),0);
    QVERIFY(captured.contains("Authorization: Bearer secret")); QVERIFY(captured.contains("Never promise returns")); QVERIFY(captured.contains("CONCENTRATION"));
    captured.clear(); invalid=true; provider.generateAsync(evidence,report); QTRY_COMPARE_WITH_TIMEOUT(failed.count(),1,2000); QVERIFY(failed.at(0).at(1).toString().contains(QStringLiteral("invalid")));
}
void AgentProviderTests::missingConfigDoesNotUseNetwork()
{ fininsight::analysis::OpenAICompatibleReviewGenerator provider({}); QSignalSpy failed(&provider,&fininsight::analysis::OpenAICompatibleReviewGenerator::failed); provider.generateAsync({},{}); QTRY_COMPARE(failed.count(),1); QVERIFY(failed.at(0).at(1).toString().contains(QStringLiteral("not configured"))); }
QTEST_GUILESS_MAIN(AgentProviderTests)
#include "agent_provider_tests.moc"
