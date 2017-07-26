LD_PRELOAD=/usr/local/lib/libtrace.so gunicorn -b :8080 --threads 4 server:app
