-- traces table
create table traces(trace_id text primary key, body json, num_spans int, duration float, ts timestamp default now());
