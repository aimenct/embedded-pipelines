export USER_ID="$(id -u)"
export USER_GID="$(id -g)"
docker compose up -d

unset USER_ID
unset USER_GID