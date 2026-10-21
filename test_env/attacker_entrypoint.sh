#!/bin/sh
set -eu

# The image is built from the local checkout and sets the project working dir.
# A failed build exits here instead of leaving an apparently healthy container.
make
exec "$@"
