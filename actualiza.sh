#!/usr/bin/env bash
set -euo pipefail

# Reescribe TODOS los commits reemplazando tu email privado
git filter-branch --env-filter '
OLD_EMAIL="donmauro.col@gmail.com"
NEW_EMAIL="220961612+DonMaurocolombia@users.noreply.github.com"
if [ "$GIT_COMMITTER_EMAIL" = "$OLD_EMAIL" ]; then
    export GIT_COMMITTER_EMAIL="$NEW_EMAIL"
fi
if [ "$GIT_AUTHOR_EMAIL" = "$OLD_EMAIL" ]; then
    export GIT_AUTHOR_EMAIL="$NEW_EMAIL"
fi
' --tag-name-filter cat -- --branches --tags

