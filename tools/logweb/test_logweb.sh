#!/bin/bash
# test_logweb.sh —— logweb 集成测试
set -e

BIN=./logweb
PORT=18765
BASE="http://127.0.0.1:$PORT"
DB=$(pwd)/../../example/data/log.db

# 启动前确保数据库存在
if [ ! -f "$DB" ]; then
    echo "SKIP: $DB not found, run example/DataBaseTest first"
    exit 0
fi

# 启动服务端（后台）
$BIN --port $PORT --db "$DB" &
PID=$!
sleep 1  # 等端口就绪

cleanup() { kill $PID 2>/dev/null; wait $PID 2>/dev/null; }
trap cleanup EXIT

PASS=0
FAIL=0
check() {
    local label="$1"
    local expected="$2"
    local actual="$3"
    if echo "$actual" | grep -Fq "$expected"; then
        echo "  ✅ $label"
        PASS=$((PASS+1))
    else
        echo "  ❌ $label (expected '$expected' not found)"
        echo "     got: $(echo "$actual" | head -c 200)"
        FAIL=$((FAIL+1))
    fi
}

echo "=== Test 1: 首页返回 HTML ==="
RESP=$(curl -s "$BASE/")
check "contains html" "<html" "$RESP"
check "contains LogSystem" "LogSystem" "$RESP"

echo "=== Test 2: GET /api/logs ==="
RESP=$(curl -s "$BASE/api/logs")
check "JSON array" "[" "$RESP"
check "contains log_level" "log_level" "$RESP"

echo "=== Test 3: GET /api/logs?level=FATAL ==="
RESP=$(curl -s "$BASE/api/logs?level=FATAL")
check "filtered to FATAL" "FATAL" "$RESP"

echo "=== Test 4: GET /api/stats?group_by=level ==="
RESP=$(curl -s "$BASE/api/stats?group_by=level")
check "has groups" "groups" "$RESP"
check "has total" "total" "$RESP"

echo "=== Test 5: GET /api/loggers ==="
RESP=$(curl -s "$BASE/api/loggers")
check "JSON array" "[" "$RESP"
check "contains Asynclogger" "Asynclogger" "$RESP"

echo "=== Test 6: 404 处理 ==="
RESP=$(curl -s --max-time 3 "$BASE/nosuch" 2>/dev/null || echo "")
check "404 body" "Not Found" "$RESP"

echo "=== Test 7: SSE /api/stream ==="
HEADERS=$(curl -sI --max-time 3 "$BASE/api/stream" 2>/dev/null || true)
check "SSE content-type" "text/event-stream" "$HEADERS"

echo ""
echo "=== Results: $PASS passed, $FAIL failed ==="
[ $FAIL -eq 0 ] && echo "ALL PASS" || echo "SOME FAILED"
exit $FAIL
