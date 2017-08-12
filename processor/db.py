from collections import deque

import psycopg2
import psycopg2.extras

from gen import request_log_pb2

import trace

conn = psycopg2.connect(
        "postgres://dciohjpo:yjfkAOIhJwx1JE-txPs1tFsdQ547RkPo@horton.elephantsql.com:5432/dciohjpo")


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
        span_obj = trace.make_span(db_id,
                span.time,
                trace.UUID(span.context.trace_id.high,
                    span.context.trace_id.low),
                trace.UUID(span.context.span_id.high,
                    span.context.span_id.low),
                trace.UUID(span.context.parent_span.high,
                    span.context.parent_span.low),
                span.conn.client_hostname,
                span.conn.server_hostname,
                span.duration,
                span.info)

        spans.append(span_obj)
    print('loaded', len(spans), 'spans')
    return spans

"""
Saves the given traces to the database.
"""
def upload(traces):
    cur = conn.cursor()
    for trace in traces:
        trace_id = trace['start']['context']['trace_id']['py/state']
        cur.execute(
                "INSERT INTO traces (body, trace_id, num_spans, duration) VALUES (%s, %s, %s, %s)",
                [psycopg2.extras.Json(trace), trace_id, trace['num_spans'], trace['duration']])
    conn.commit()
    print('uploaded', len(traces), 'traces')
    cur.close()

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

"""
Deletes spans from the database with the given IDs
"""
def delete_spans(ids):
    if not ids:
        return
    cur = conn.cursor()
    cur.execute("DELETE FROM spans WHERE id IN %s", [tuple(ids)])
    conn.commit()
    cur.close()
    print('deleted', len(ids), 'spans')

"""
Deletes traces from the database with the given IDs
"""
def delete_traces(trace_ids):
    if not trace_ids:
        return
    cur = conn.cursor()
    cur.execute("DELETE FROM traces WHERE trace_id IN %s",
            [tuple([str(id) for id in trace_ids])])
    conn.commit()
    cur.close()
    print('deleted', len(trace_ids), 'traces')

def get_trace(trace_id):
    cur = conn.cursor()
    cur.execute("SELECT * FROM traces WHERE trace_id=%s", [str(trace_id)])
    trace = cur.fetchone()
    return trace

