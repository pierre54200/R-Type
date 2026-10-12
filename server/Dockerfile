FROM gcc:14

RUN apt-get update \
    && apt-get install -y --no-install-recommends inotify-tools cmake ninja-build \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY app/ /app/


CMD ["bash", "/app/watch.sh"]