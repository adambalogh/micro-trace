-- traces table
create table traces(
    trace_id text primary key,
    body json,
    num_spans int,
    duration float,
    ts timestamp default now()
);

-- spans table
create table spans(
    id int serial,
    json bodya -- TODO this should not be called json, its protobuf
);
