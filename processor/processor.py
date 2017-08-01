import struct
import jsonpickle
import json

from pymongo import MongoClient
import psycopg2
import psycopg2.extras

from trace import *
from gen import request_log_pb2

conn = psycopg2.connect(
        "postgres://dciohjpo:yjfkAOIhJwx1JE-txPs1tFsdQ547RkPo@horton.elephantsql.com:5432/dciohjpo")

def upload(traces):
    cur = conn.cursor()
    for trace in traces:
        cur.execute("INSERT INTO traces (body, num_spans, duration) VALUES (%s, %s, %s)",
                [psycopg2.extras.Json(trace), trace['num_spans'], trace['duration']])
    conn.commit()
    cur.close()
    conn.close()

def load_spans():
    cur = conn.cursor()
    cur.execute("SELECT * FROM spans")
    protos = cur.fetchall()
    cur.close()
    conn.commit()

    spans = []
    for proto in protos:
        span = request_log_pb2.RequestLog()
        ret = span.ParseFromString(proto[1])

        # Convert into custom Span obj
        span_obj = make_span(span.time,
                UUID(span.context.trace_id.high,
                    span.context.trace_id.low),
                UUID(span.context.span_id.high,
                    span.context.span_id.low),
                UUID(span.context.parent_span.high,
                    span.context.parent_span.low),
                span.conn.client_hostname,
                span.conn.server_hostname)

        spans.append(span_obj)
    return spans

def group_spans(spans):
    spans_map = {}
    for s in spans:
        if s.trace() not in spans_map:
            spans_map[s.trace()] = []
        spans_map[s.trace()].append(s)
    return spans_map

def process_trace(spans):
    span_id = {}
    for span in spans:
        span_id[span.span()] = span

    start = None
    for span in spans:
        parent_span = span.parent_span()

        # We found the first span in the trace
        if parent_span == span.trace():
            assert start == None
            start = span
            continue

        # error
        if parent_span not in span_id:
            continue

        span_id[parent_span].add_callee(span)

    return Trace(start)

def process(trace_id_map):
    traces = []
    for trace_id in trace_id_map.keys():
        traces.append(process_trace(trace_id_map[trace_id]))
    for trace in traces:
        post_process(trace)
    return traces

def post_process(trace):
    (num_spans, max_end) = traverse(trace.start)
    trace.num_spans = num_spans
    trace.duration = max_end - trace.start.time

def traverse(span):
    num_spans = 1
    max_end = span.time
    for c in span.callees:
        (n, m) = traverse(c)
        num_spans += n
        max_end = max(max_end, m)

    return (num_spans, max_end)



# import test
# spans = test.spans

if __name__ == "__main__":
    spans = load_spans()
    trace_id_map = group_spans(spans)
    traces = process(trace_id_map)
    data = json.loads(jsonpickle.encode(traces, unpicklable=False))
    upload(data)
