echo 'running Processor at' $(date)
/usr/local/bin/python /usr/src/app/processor.py  >> /var/log/cron.log 2>&1
