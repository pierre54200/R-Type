FROM gcc:14

RUN apt-get update \
    && apt-get install -y --no-install-recommends inotify-tools cmake ninja-build libgtest-dev gcovr \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copie de base (remplacee par le volume en dev)
COPY app/ /app/

# Le port est defini dans le .env (transmis par docker-compose)

# Lance watch.sh depuis /app : comme /app est monte en volume,
# une modification de watch.sh est prise en compte au prochain demarrage sans rebuild
CMD ["bash", "/app/watch.sh"]