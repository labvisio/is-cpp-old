FROM ubuntu:16.04
ADD ./app /app/
#ADD ./test /usr/lib/x86_64-linux-gnu/
ENTRYPOINT ["/app"]