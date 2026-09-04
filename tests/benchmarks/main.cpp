#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QVector>
#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QDateTime>

#include "data/ringbuffer.h"
#include "protocol/crc16.h"
#include "protocol/binaryframebuilder.h"
#include "protocol/binaryframeparser.h"
#include "protocol/modbusrtuprotocol.h"
#include "protocol/modbustcpprotocol.h"
#include "data/datapersistent.h"

#include <cstdio>
#include <algorithm>

// ==================================================================
// Benchmark 1: RingBuffer write+read throughput (single-thread)
// ==================================================================
static void benchRingBuffer()
{
    const int N = 1000000;
    const size_t CAP = 10000;
    RingBuffer<uint64_t> rb(CAP, true);

    QElapsedTimer timer;
    timer.start();

    for (int i = 0; i < N; ++i)
        rb.write(static_cast<uint64_t>(i));

    uint64_t val = 0;
    int readCount = 0;
    while (rb.read(val))
        ++readCount;

    qint64 elapsed = timer.elapsed();
    double seconds = elapsed / 1000.0;
    double throughput = (N + readCount) / seconds;

    printf("===== Benchmark 1: RingBuffer Write+Read Throughput =====\n");
    printf("  Capacity       : %zu\n", CAP);
    printf("  Operations     : %d write + %d read = %d total\n", N, readCount, N + readCount);
    printf("  Elapsed        : %lld ms (%.3f s)\n", elapsed, seconds);
    printf("  Throughput     : %.0f ops/sec\n", throughput);
    printf("  Data points/sec: %.0f (write-only single direction)\n", N / seconds);
    printf("  Verify         : written=%d read=%d %s\n",
           N, readCount,
           (readCount == static_cast<int>(CAP)) ? "OK" : "CHECK");
    printf("\n");
    fflush(stdout);
}

// ==================================================================
// Benchmark 2: Protocol parse rate
// ==================================================================
static void benchProtocolParse()
{
    const int N = 10000;

    QVector<ModbusRequest> rtuRequests;
    for (int i = 0; i < N; ++i) {
        ModbusRequest req;
        req.slaveAddress = static_cast<quint8>((i % 247) + 1);
        req.functionCode = ModbusFunctionCode::ReadHoldingRegisters;
        req.startAddress = static_cast<quint16>(i * 2);
        req.quantity = 10;
        rtuRequests.append(req);
    }

    QVector<QByteArray> binPayloads;
    for (int i = 0; i < N; ++i) {
        QByteArray payload;
        payload.append(static_cast<char>(0));
        payload.append(static_cast<char>(2));
        float val = 123.456f + i;
        QByteArray fb(reinterpret_cast<const char *>(&val), 4);
        std::reverse(fb.data(), fb.data() + 4);
        payload.append(fb);
        binPayloads.append(payload);
    }

    // Prepare Modbus TCP responses (correct MBAP format)
    ModbusTcpProtocol tcpProto(1);
    QVector<QByteArray> tcpResponses;
    for (int i = 0; i < N; ++i) {
        quint8 unit = static_cast<quint8>((i % 247) + 1);
        // MBAP: TID(2B) + PID(2B=0) + Length(2B) + UnitID(1B)
        quint16 tid = static_cast<quint16>(i);
        QByteArray resp;
        resp.append(static_cast<char>(tid >> 8));
        resp.append(static_cast<char>(tid & 0xFF));
        resp.append(static_cast<char>(0));  // Protocol ID high
        resp.append(static_cast<char>(0));  // Protocol ID low
        // Length = UnitID(1) + FC(1) + byteCount(1) + data(20) = 23
        resp.append(static_cast<char>(0));
        resp.append(static_cast<char>(23));
        resp.append(static_cast<char>(unit));
        resp.append(static_cast<char>(0x03));  // FC
        resp.append(static_cast<char>(20));    // byte count
        for (int j = 0; j < 10; ++j) {
            quint16 v = static_cast<quint16>(j * 100 + i);
            resp.append(static_cast<char>(v >> 8));
            resp.append(static_cast<char>(v & 0xFF));
        }
        tcpResponses.append(resp);
    }

    QElapsedTimer timer;

    // --- Modbus RTU parse ---
    timer.start();
    int rtuOk = 0;
    for (int i = 0; i < N; ++i) {
        quint8 slave = static_cast<quint8>((i % 247) + 1);
        QByteArray resp;
        resp.append(static_cast<char>(slave));
        resp.append(static_cast<char>(0x03));
        resp.append(static_cast<char>(20));
        for (int j = 0; j < 10; ++j) {
            quint16 v = static_cast<quint16>(j * 100 + i);
            resp.append(static_cast<char>(v >> 8));
            resp.append(static_cast<char>(v & 0xFF));
        }
        quint16 crc = Crc16::calculate(resp);
        resp.append(static_cast<char>(crc & 0xFF));
        resp.append(static_cast<char>(crc >> 8));
        ModbusResponse mr = ModbusRtuProtocol::parseResponse(resp, rtuRequests[i]);
        if (!mr.isException) ++rtuOk;
    }
    qint64 rtuElapsed = timer.elapsed();

    // --- BinaryFrame parse ---
    timer.restart();
    int binOk = 0;
    for (int i = 0; i < N; ++i) {
        QByteArray fullFrame = BinaryFrameBuilder::buildFrame(binPayloads[i]);
        BinaryFrameParser parser;
        int received = 0;
        QObject::connect(&parser, &BinaryFrameParser::frameReceived,
            [&received](const QByteArray &) { ++received; });
        parser.feedData(fullFrame);
        if (received == 1) ++binOk;
    }
    qint64 binElapsed = timer.elapsed();

    // --- Modbus TCP parse ---
    timer.restart();
    int tcpOk = 0;
    for (int i = 0; i < N; ++i) {
        ModbusResponse mr = ModbusTcpProtocol::parseResponseRaw(tcpResponses[i]);
        if (!mr.isException && mr.data.size() == 10) ++tcpOk;
    }
    qint64 tcpElapsed = timer.elapsed();

    double rtuSec = (rtuElapsed > 0 ? rtuElapsed : 1) / 1000.0;
    double binSec = (binElapsed > 0 ? binElapsed : 1) / 1000.0;
    double tcpSec = (tcpElapsed > 0 ? tcpElapsed : 1) / 1000.0;

    printf("===== Benchmark 2: Protocol Parse Rate (%d frames each) =====\n", N);
    printf("  Modbus RTU   : %lld ms (%.3f s) -> %.0f frames/sec  (parsed %d/%d OK)\n",
           rtuElapsed, rtuSec, N / rtuSec, rtuOk, N);
    printf("  BinaryFrame  : %lld ms (%.3f s) -> %.0f frames/sec  (parsed %d/%d OK)\n",
           binElapsed, binSec, N / binSec, binOk, N);
    printf("  Modbus TCP   : %lld ms (%.3f s) -> %.0f frames/sec  (parsed %d/%d OK)\n",
           tcpElapsed, tcpSec, N / tcpSec, tcpOk, N);
    qint64 totalElapsed = rtuElapsed + binElapsed + tcpElapsed;
    if (totalElapsed == 0) totalElapsed = 1;
    printf("  Total 3proto : %lld ms -> %.0f frames/sec (combined)\n",
           totalElapsed, (3.0 * N) / (totalElapsed / 1000.0));
    printf("\n");
    fflush(stdout);
}

// ==================================================================
// Benchmark 3: SQLite insert + query
// ==================================================================
static void benchSqlite()
{
    const int INSERT_SINGLE = 10000;
    const int INSERT_BATCH  = 10000;
    const int QUERY_COUNT   = 1000;

    QString dbPath = QDir::tempPath() + "/bench_debugplatform.db";
    QFile::remove(dbPath);

    DataPersistent db;
    if (!db.initialize(dbPath)) {
        printf("===== Benchmark 3: SQLite (FAILED to init) =====\n\n");
        return;
    }
    db.addDevice("bench_dev", "Benchmark Device", "TCP", "{}");

    QElapsedTimer timer;

    // --- Single insert ---
    timer.start();
    for (int i = 0; i < INSERT_SINGLE; ++i)
        db.insertRegisterData("bench_dev", static_cast<quint16>(i % 100), 123.456 + i);
    qint64 singleElapsed = timer.elapsed();

    // --- Batch insert ---
    timer.restart();
    QVector<QPair<QString, QPair<quint16, double>>> batch;
    batch.reserve(INSERT_BATCH);
    for (int i = 0; i < INSERT_BATCH; ++i) {
        batch.append(qMakePair(QString("bench_dev"),
                    qMakePair(static_cast<quint16>(i % 100), 789.012 + i)));
    }
    db.insertRegisterDataBatch(batch);
    qint64 batchElapsed = timer.elapsed();

    // --- Query ---
    timer.restart();
    int totalQueried = 0;
    QDateTime now = QDateTime::currentDateTimeUtc();
    QDateTime past = now.addDays(-1);
    for (int i = 0; i < QUERY_COUNT; ++i) {
        quint16 addr = static_cast<quint16>(i % 100);
        auto results = db.queryRegisterData("bench_dev", addr, past, now);
        totalQueried += results.size();
    }
    qint64 queryElapsed = timer.elapsed();

    double singleSec = singleElapsed / 1000.0;
    double batchSec  = batchElapsed / 1000.0;
    double queryAvgMs = static_cast<double>(queryElapsed) / QUERY_COUNT;

    printf("===== Benchmark 3: SQLite Insert + Query =====\n");
    printf("  Single insert : %d rows in %lld ms (%.3f s) -> %.0f rows/sec\n",
           INSERT_SINGLE, singleElapsed, singleSec, INSERT_SINGLE / singleSec);
    printf("  Batch insert   : %d rows in %lld ms (%.3f s) -> %.0f rows/sec\n",
           INSERT_BATCH, batchElapsed, batchSec, INSERT_BATCH / batchSec);
    printf("  Batch vs Single: %.1fx faster\n",
           static_cast<double>(singleElapsed) / batchElapsed);
    printf("  Query          : %d queries in %lld ms -> avg %.3f ms/query (retrieved %d rows)\n",
           QUERY_COUNT, queryElapsed, queryAvgMs, totalQueried);
    printf("\n");
    fflush(stdout);

    QFile::remove(dbPath);
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    printf("=========================================================\n");
    printf(" DebugPlatform Benchmark Suite (C++11 / Qt %s)\n", QT_VERSION_STR);
    printf(" Built: %s %s\n", __DATE__, __TIME__);
    printf("=========================================================\n\n");
    fflush(stdout);

    benchRingBuffer();
    benchProtocolParse();
    benchSqlite();

    printf("=========================================================\n");
    printf(" All benchmarks completed.\n");
    printf("=========================================================\n");

    return 0;
}