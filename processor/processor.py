import struct
import jsonpickle
from jsonpickle import unpickler
import json
from collections import deque

from pymongo import MongoClient
import psycopg2
import psycopg2.extras

from trace import *
from gen import request_log_pb2

conn = psycopg2.connect(
        "postgres://dciohjpo:yjfkAOIhJwx1JE-txPs1tFsdQ547RkPo@horton.elephantsql.com:5432/dciohjpo")


"""
Returns the 1000 most recent traces from the database.
"""
def get_recent_traces():
    cur = conn.cursor()
    cur.execute("SELECT * FROM traces ORDER BY ts DESC LIMIT 1000");
    traces = cur.fetchall()
    print 'fetched', len(traces), 'traces'
    conn.commit()
    cur.close()
    return {trace.start.trace_id: trace for trace in traces}

"""
Saves the given traces to the database.
"""
def upload(traces):
    cur = conn.cursor()
    print 'uploading', len(traces), 'traces'
    for trace in traces:
        trace_id = trace['start']['context']['trace_id']['py/state']
        cur.execute(
                "INSERT INTO traces (body, trace_id, num_spans, duration) VALUES (%s, %s, %s, %s)",
                [psycopg2.extras.Json(trace), trace_id, trace['num_spans'], trace['duration']])
    conn.commit()
    cur.close()

"""
Returns all the spans from the database.
"""
def load_spans():
    cur = conn.cursor()
    cur.execute("SELECT * FROM spans")
    protos = cur.fetchall()
    conn.commit()
    cur.close()

    spans = []
    for proto in protos:
        span = request_log_pb2.RequestLog()
        db_id = proto[0]
        ret = span.ParseFromString(proto[1])

        # Convert into custom Span obj
        span_obj = make_span(db_id,
                span.time,
                UUID(span.context.trace_id.high,
                    span.context.trace_id.low),
                UUID(span.context.span_id.high,
                    span.context.span_id.low),
                UUID(span.context.parent_span.high,
                    span.context.parent_span.low),
                span.conn.client_hostname,
                span.conn.server_hostname,
                span.duration)

        spans.append(span_obj)
    print 'loaded', len(spans), 'spans'
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
    traces = [trace for trace in traces if trace.start is not None]
    for trace in traces:
        post_process(trace)
    return traces

def post_process(trace):
    (num_spans, max_end) = traverse(trace.start)
    trace.num_spans = num_spans
    trace.duration = trace.start.duration

def traverse(span):
    num_spans = 1
    max_end = span.time
    for c in span.callees:
        (n, m) = traverse(c)
        num_spans += n
        max_end = max(max_end, m)

    return (num_spans, max_end)

"""
Returns the database IDs of the spans that are part of the traces that will
be uploaded to the database
"""
def get_db_ids(traces):
    ids = set()
    q = deque()
    for trace in traces:
        q.append(trace.start)
    while q:
        span = q.pop()
        ids.add(span.db_id)
        for c in span.callees:
            q.append(c)
    return ids

def delete_spans(ids):
    if not ids:
        return
    cur = conn.cursor()
    cur.execute("DELETE FROM spans WHERE id IN %s", [tuple(ids)])
    conn.commit()
    cur.close()
    print 'deleted', len(ids), 'spans'

def delete_traces(trace_ids):
    if not ids:
        return
    cur = conn.cursor()
    cur.execute("DELETE FROM traces WHERE trace_id IN %s",
            [tuple([str(id) for id in ids])])
    conn.commit()
    cur.close()
    print 'deleted', len(ids), 'traces'

def get_trace(trace_id):
    cur = conn.cursor()
    cur.execute("SELECT * FROM traces WHERE trace_id=%s", [str(trace_id)])
    trace = cur.fetchone()
    return trace

def unprocessed_traces(trace_id_map):
    db_ids = set()
    traces = []
    for trace_id in trace_id_map.keys():
        trace = get_trace(trace_id)
        if trace:
            (ids, trace) = extend_trace(trace[2], trace_id_map[trace_id])
            if ids:
                db_ids.update(ids)
                traces.append(trace)
    return (db_ids, traces)

def extend_trace(old_trace_json, spans):
    u = unpickler.Unpickler()
    trace = u.restore(old_trace_json)
    span_map = {}
    q = deque()
    q.append(trace.start)
    while q:
        span = q.pop()
        span_map[span.context.span_id] = span
        for c in span.callees:
            q.append(c)
    ids = set()
    for span in spans:
        if span.context.parent_span in span_map:
            parent_span = span_map[span.context.parent_span]
            parent_span.add_callee(span)
            ids.add(span.db_id)
    return (ids, trace)

if __name__ == "__main__":
    # get spans
    spans = load_spans()

    # build traces
    trace_id_map = group_spans(spans)
    traces = process(trace_id_map)

    # delete used spans from db
    db_ids = get_db_ids(traces)
    delete_spans(db_ids)

    # upload traces to db
    data = json.loads(jsonpickle.encode(traces))
    upload(data)

    # try to insert unused spans into existing traces from db
    spans = load_spans()
    trace_id_map = group_spans(spans)
    print 'got', len(trace_id_map), 'traces unprocessed'
    (ids, new_traces) = unprocessed_traces(trace_id_map)
    print 'extended', len(new_traces), 'traces'
    for trace in new_traces:
        post_process(trace)
    data = json.loads(jsonpickle.encode(new_traces))
    upload(data)

    delete_spans(ids)
    delete_traces([trace.start.context.trace_id for trace in new_traces])


