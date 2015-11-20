# Travis-CI wrapper for parallel builds
if [ $# -eq 3 ] ; then
    make build-$1 os=$2 dist=$3
    #make export-$1 is=$2 dist=$3
else
    echo 'build skipped'
    echo 'Usage:'
    echo './pack.sh [rpm/deb] <os> <distr>'
    echo 'Example: ./pack.sh rpm fedora 20'
    exit 0
fi
