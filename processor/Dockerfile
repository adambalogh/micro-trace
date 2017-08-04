FROM python:3

RUN apt-get update && apt-get install -y cron

WORKDIR /usr/src/app

COPY docker/requirements.txt .
COPY docker/run.sh .

COPY processor.py .
COPY trace.py .
COPY db.py .

RUN mkdir gen
COPY gen/request_log_pb2.py gen/.

RUN pip install --no-cache-dir -r requirements.txt

# Add crontab file in the cron directory
ADD docker/crontab /etc/cron.d/hello-cron
 
# Give execution rights on the cron job
RUN chmod 0644 /etc/cron.d/hello-cron
 
# Create the log file to be able to run tail
RUN touch /var/log/cron.log
 
# Run the command on container startup
CMD cron && tail -f /var/log/cron.log
